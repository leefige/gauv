#pragma once

#include <chrono>

#include <immer/set.hpp>

#include <spdlog/spdlog.h>
#include <omp.h>

#include <functional>
#include <vector>
#include <unordered_set>

#include "graph.hpp"
#include "transformer.hpp"
#include "common.hpp"

namespace mpc {

class Prover {
    const GraphBase& graph_base;
    const std::vector<std::unordered_set<PartyDecl*>> equivalent_classes;
    const std::vector<PartyDecl*> parties;
    const unsigned T;

    std::function<bool(Graph)> algorithm; // 这样写是为了更灵活，将来可以方便地替换这个核心算法

    std::vector<Graph> possible_start_graphs; // graph_base + possible corrupted party set
    void search_all_possibilities(
            unsigned equivalent_class_id,
            unsigned corrupted_quota,
            immer::set<PartyDecl*> corrupted_parties,
            bool is_quota_strict) {
        assert(corrupted_quota >= 0);

        if (equivalent_class_id == equivalent_classes.size()) {
            if (corrupted_parties.size() > 0
                && ((is_quota_strict && corrupted_quota == 0) || !is_quota_strict)) {
                    possible_start_graphs.push_back(
                        Graph(graph_base, parties.size(), T, corrupted_parties));
                }
            return;
        }

        // 先来处理在这个等价类里一个也不选的情况
        search_all_possibilities(
            equivalent_class_id + 1,
            corrupted_quota,
            corrupted_parties,
            is_quota_strict);
        unsigned i = 0;
        for (auto party: equivalent_classes[equivalent_class_id]) {
            if (i == corrupted_quota) break;
            
            ++i;
            corrupted_parties = corrupted_parties.insert(party);

            // 在这个等价类里面选 i 个 corrupted_aprty
            search_all_possibilities(
                equivalent_class_id + 1,
                corrupted_quota - i,
                corrupted_parties,
                is_quota_strict);
        }
    }
public:
    // 俺们现在的算法是这样的，每次选一个让秩函数下降最多的 transformation 来做。
    static bool tryProving(Graph g, const std::vector<PartyDecl*> parties) {
        spdlog::info("Consider the case that the corrupted parties are (T = {}):", g.T);
        for (auto party: parties)
            if (g.corruptedParties.find(party) != nullptr)
                spdlog::info("\t{}", party->to_string());

        // 定义 equivalent rewriters
        Transformer* additionRewriter = new AdditionTransformer();

        // ** 设置 source parties
        std::unordered_set<PartyDecl*> srcParties;
        for (auto party: parties) {
            if (g.corruptedParties.find(party) != nullptr) // corrupted parties
                srcParties.insert(party);
        }
        // ** 加入 honest party 作为 source party 直到到达阈值 T
        for (auto party: parties) {
            if (srcParties.size() == g.T)
                break;
            if (g.corruptedParties.find(party) == nullptr) // the party is honest
                srcParties.insert(party);
        }
        Transformer* sharingRewriter = new SharingTransformer(srcParties);
        Transformer* reconstructionRewriter1 = new ReconstructionTransformer(srcParties);

        // ** 再多加一个 honest party 作为 source party，使得我们有 T + 1 个 source parties
        for (auto party: parties) {
            if (g.corruptedParties.find(party) == nullptr // the party is honest
                && !srcParties.contains(party)) {
                srcParties.insert(party);
                break;
            }
        }
        Transformer* randomSharingRewriter = new RandomSharingTransformer(srcParties);
        Transformer* reconstructionRewriter2 = new ReconstructionTransformer(srcParties);

        std::vector<Transformer*> equivalent_rewriters{
            additionRewriter,
            sharingRewriter,
            randomSharingRewriter,
            reconstructionRewriter1,
            reconstructionRewriter2
        };

        std::unordered_set<Graph, Graph::Hash> considered_graphs;
        while (g.hasBubble()) {
            bool transformed = false;
            std::shared_ptr<Node> min_node = nullptr;
            std::string transformation_type = "";
            Graph min_new_g;
            for (auto& transformer : equivalent_rewriters) {
                for (auto& [node, new_g]: transformer->apply(g)) {
                    // spdlog::trace("{} considers {}, potential becomes {}, hash becomes {}", transformer->to_string(), node->to_string(), to_string(new_g.potential()), new_g.hash);

                    if (new_g.potential() < g.potential() &&
                        (min_node == nullptr || new_g.potential() < min_new_g.potential()) &&
                        !considered_graphs.contains(new_g)) {
                        min_node = std::move(node);
                        min_new_g = std::move(new_g);
                        transformation_type = transformer->to_string();
                        transformed = true;
                        break;
                    }
                }
            }

            if (!transformed) break;
            g = std::move(min_new_g);
            considered_graphs.insert(g);

            spdlog::debug("{} {} {} {}",
                min_node->getName(),
                transformation_type,
                to_string(g.potential()),
                g.hash
            );
            // spdlog::debug("After rewriting, the graph becomes\n{}", g.to_string());
        }

        // Actually, eliminating tail nodes won't make difference to the equivalent rewriting.
        // Thus, we could move all tail node eliminations to the end.
        while (true) {
            bool transformed = false;
            for (size_t node_id = 0; node_id < g.nodeSize(); ++node_id)
                if (g.nodes[node_id] != nullptr) { // 为空表示这个节点已经被删掉了
                    if (g.outDeg(node_id) == 0 && g.corruptedParties.find(g.nodes[node_id]->party) == nullptr) { // 我们删掉 honest party 中出度为 0 的节点
                        g = g.eliminateNode(g.nodes[node_id]);

                        transformed = true;
                        break;
                    }
                }
            if (!transformed) break;
        }

        spdlog::info("Final graph has {} nodes, {} edges.", g.nodeSize(), g.edgeSize());
        // spdlog::info("Final potential: {}", to_string(g.potential()));
        // spdlog::debug("Final graph:\n{}", g.to_string());

        spdlog::info("Proved? {}", !g.hasBubble());

        return !g.hasBubble();
    }

    Prover(
        const GraphBase& graph_base,
        std::vector<std::unordered_set<PartyDecl*>> equivalent_classes,
        std::vector<PartyDecl*> parties,
        const unsigned T
    ): graph_base(graph_base), equivalent_classes(equivalent_classes), parties(parties), T(T) {
        algorithm = std::bind(tryProving, std::placeholders::_1, parties);

        spdlog::info("Initial graph has {} nodes, {} edges.", graph_base.nodeSize(), graph_base.edgeSize());
        // spdlog::debug("Initial graph:\n{}", graph_base.to_string());
    }
    ~Prover() {}

    void try_all_possibilities() {
        bool proved = true;
#ifdef _OPENMP
#pragma omp parallel for num_threads(omp_get_num_procs()) reduction(&:proved)
#endif
        for (size_t i = 0; i < possible_start_graphs.size(); ++i) {
            proved &= algorithm(possible_start_graphs[i]);
        }
        spdlog::info("Overall, Proved? {}", proved);
    }

    void prove(unsigned I) {
        possible_start_graphs.clear();

        search_all_possibilities(0, I, immer::set<PartyDecl*>(), true);

        try_all_possibilities();
    }
    void prove() {
        possible_start_graphs.clear();

        search_all_possibilities(0, T, immer::set<PartyDecl*>(), false);

        try_all_possibilities();
    }

    // old function, updates required
    // void initSearchState() {
    //     for (auto nd : nodes) {
    //         if (nd->getInDegrees() == 0 && !nd->party->is_corrupted() &&
    //             nd->type != Node::RANDOM) {
    //             nd->state = Node::BUBBLE;
    //             bubbleSet.insert(nd);
    //             searchSet.insert(nd);
    //         } else if (nd->getOutDegrees() == 0) {
    //             nd->state = Node::POTENTIAL;
    //             searchSet.insert(nd);
    //         } else {
    //             nd->state = Node::UNVISITED;
    //         }
    //     }
    //     for (auto edge : edges) {
    //         edge->state = Operation::UNVISITED;
    //     }
    //     transformTape.clear();
    //     srcParties.clear();
    // }

    // old function, updates required
    // GraphProver* tryProving(std::vector<std::string>& histories) {
    //     if (!hasBubble()) {
    //         return this;
    //     }

    //     std::string hashString = getHashString();
    //     for (auto history : histories) {
    //         if (history == hashString) {
    //             return nullptr;
    //         }
    //     }

    //     histories.push_back(hashString);
    //     GraphProver* simulator = nullptr;

    //     for (int i = 0; i < nodes.size(); i++) {
    //         if (nodes[i]->state == Node::POTENTIAL ||
    //             nodes[i]->state == Node::BUBBLE) {
    //             GraphProver* g_1 = new GraphProver(*this);
    //             if (g_1->eliminateTailingNode(g_1->nodes[i])) {
    //                 std::cout << "TAIL_NODE" << std::endl;
    //                 if ((simulator = g_1->tryProving(histories)) != nullptr) {
    //                     return simulator;
    //                 }
    //             }
    //         }
    //     }

    //     for (int i = 0; i < nodes.size(); i++) {
    //         if (nodes[i]->state != Node::ELIMINATED) {
    //             GraphProver* g_2 = new GraphProver(*this);
    //             if (g_2->reverseOutputReconstruct(g_2->nodes[i])) {
    //                 std::cout << "REVERSE_OUTPUT_RECONSTRUCT" << std::endl;
    //                 if ((simulator = g_2->tryProving(histories)) != nullptr) {
    //                     return simulator;
    //                 }
    //             }
    //         }
    //     }

    //     for (int i = 0; i < nodes.size(); i++) {
    //         if (nodes[i]->state != Node::ELIMINATED) {
    //             GraphProver* g_3 = new GraphProver(*this);
    //             if (g_3->simulatePolynomial(g_3->nodes[i])) {
    //                 std::cout << "SIM_POLY" << std::endl;
    //                 if ((simulator = g_3->tryProving(histories)) != nullptr) {
    //                     return simulator;
    //                 }
    //             }
    //             GraphProver* g_4 = new GraphProver(*this);
    //             if (g_4->reverseReconstruct(g_4->nodes[i])) {
    //                 std::cout << "REVERSE_RECONSTRUCT" << std::endl;
    //                 if ((simulator = g_4->tryProving(histories)) != nullptr) {
    //                     return simulator;
    //                 }
    //             }
    //             GraphProver* g_5 = new GraphProver(*this);
    //             if (g_5->reverseAddition(g_5->nodes[i])) {
    //                 std::cout << "REVERSE_ADDITION" << std::endl;
    //                 if ((simulator = g_5->tryProving(histories)) != nullptr) {
    //                     return simulator;
    //                 }
    //             }
    //         }
    //     }

    //     std::cout << "GO_BACK" << std::endl;
    //     return nullptr;
    // }

    // old function, updates required
    // bool tryProvingByPotential() {
    //     computePotential = true;
    //     while (hasBubble()) {
    //         bool transformed = false;
    //         // first try to eliminate tailing node
    //         for (auto node : nodes)
    //             if (node->state == Node::POTENTIAL ||
    //                 node->state == Node::BUBBLE)
    //                 if (eliminateTailingNode(node)) {
    //                     transformed = true;
    //                 }
    //         if (transformed) continue;
    //         for (auto node : nodes)
    //             if (node->state != Node::ELIMINATED) {
    //                 if (reverseOutputReconstruct(node)) {
    //                     transformed = true;
    //                 }
    //             }
    //         if (transformed) continue;
    //         for (auto node : nodes)
    //             if (node->state != Node::ELIMINATED) {
    //                 if (simulatePolynomial(node) || reverseReconstruct(node) || reverseAddition(node)) {
    //                     transformed = true;
    //                     break;
    //                 }
    //             }
    //         if (!transformed) {
    //             return false;
    //         }
    //     }
    //     return true;
    // }

    // old function, updates required
    // bool tryProvingByHint() {
    //     computePotential = false;
    //     while (hasBubble()) {
    //         bool transformed = false;
    //         while (!searchSet.empty()) {
    //             auto node = *searchSet.begin();
    //             searchSet.erase(node);
    //             if (!node->isEliminated()) {
    //                 if (eliminateTailingNode(node) ||
    //                     reverseOutputReconstruct(node) ||
    //                     simulatePolynomial(node)) {
    //                     transformed = true;
    //                     break;
    //                 }
    //             }
    //         }
    //         if (!transformed) return false;
    //     }
    //     return true;
    // }


    // old function, updates required
    // inline void prove(GraphProver& graph, int verbose = 3) {
    //     graph.computePotential = true;
    //     graph.initOutputNodes();
    //     graph.initSearchState();
    //     cout << "Initial graph has " << graph.nodeSize() << " nodes, "
    //         << graph.edgeSize() << " edges" << endl;
    //     cout << "Initial potential: " << GraphProver::to_string(graph.potential())
    //         << endl;
    //     if (verbose >= 3) cout << endl << "Initial graph:" << endl << graph << endl;
    //     auto begin_time = chrono::high_resolution_clock::now();
    //     vector<string> histories;
    //     bool proved = graph.tryProving(histories);
    //     auto end_time = chrono::high_resolution_clock::now();
    //     cout << endl << "Proved? " << std::boolalpha << proved << endl;
    //     cout << "Time: " << (end_time - begin_time).count() / 1e9 << endl;
    //     cout << "Final graph has " << graph.nodeSize() << " nodes, "
    //         << graph.edgeSize() << " edges" << endl;
    //     cout << "Final potential: " << GraphProver::to_string(graph.potential()) << endl;
    //     if (verbose >= 3) cout << "Final graph:" << endl << graph << endl;
    //     if (verbose >= 2) {
    //         cout << endl << "Transform history:" << endl;
    //         for (auto& entry : graph.transformTape) {
    //             cout << entry.node->getName() << " " << GraphProver::to_string(entry.type)
    //                 << " " << GraphProver::to_string(entry.potential) << endl;
    //         }
    //     }
    // }

    // old function, updates required
    // inline void prove_by_potential(GraphProver& graph, int verbose = 3) {
    //     graph.computePotential = true;
    //     graph.initOutputNodes();
    //     graph.initSearchState();
    //     cout << "Initial graph has " << graph.nodeSize() << " nodes, "
    //         << graph.edgeSize() << " edges" << endl;
    //     cout << "Initial potential: " << GraphProver::to_string(graph.potential())
    //         << endl;
    //     if (verbose >= 3) cout << endl << "Initial graph:" << endl << graph << endl;
    //     auto begin_time = chrono::high_resolution_clock::now();
    //     bool proved = graph.tryProvingByPotential();
    //     auto end_time = chrono::high_resolution_clock::now();
    //     cout << endl << "Proved? " << std::boolalpha << proved << endl;
    //     cout << "Time: " << (end_time - begin_time).count() / 1e9 << endl;
    //     cout << "Final graph has " << graph.nodeSize() << " nodes, "
    //         << graph.edgeSize() << " edges" << endl;
    //     cout << "Final potential: " << GraphProver::to_string(graph.potential()) << endl;
    //     if (verbose >= 3) cout << "Final graph:" << endl << graph << endl;
    //     if (verbose >= 2) {
    //         cout << endl << "Transform history:" << endl;
    //         for (auto& entry : graph.transformTape) {
    //             cout << entry.node->getName() << " " << GraphProver::to_string(entry.type)
    //                 << " " << GraphProver::to_string(entry.potential) << endl;
    //         }
    //     }
    // }

    // old function, updates required
    // inline void prove_by_hint(GraphProver& graph, int verbose = 3) {
    //     graph.initOutputNodes();
    //     graph.initSearchState();
    //     cout << "Initial graph has " << graph.nodeSize() << " nodes, "
    //         << graph.edgeSize() << " edges" << endl;
    //     graph.computePotential = true;
    //     cout << "Initial potential: " << GraphProver::to_string(graph.potential())
    //         << endl;
    //     if (verbose >= 3) cout << endl << "Initial graph:" << endl << graph << endl;
    //     graph.computePotential = false;
    //     auto begin_time = chrono::high_resolution_clock::now();
    //     bool proved = graph.tryProvingByHint();
    //     auto end_time = chrono::high_resolution_clock::now();
    //     cout << endl << "Proved? " << std::boolalpha << proved << endl;
    //     cout << "Time: " << (end_time - begin_time).count() / 1e9 << endl;
    //     cout << "Final graph has " << graph.nodeSize() << " nodes, "
    //         << graph.edgeSize() << " edges" << endl;
    //     graph.computePotential = true;
    //     cout << "Final potential: " << GraphProver::to_string(graph.potential()) << endl;
    //     if (verbose >= 3) cout << "Final graph:" << endl << graph << endl;
    //     if (verbose >= 2) {
    //         cout << endl << "Transform history:" << endl;
    //         for (auto& entry : graph.transformTape) {
    //             cout << entry.node->getName() << " " << GraphProver::to_string(entry.type)
    //                 << " " << GraphProver::to_string(entry.potential) << endl;
    //         }
    //     }
    // }
};

} // end of namespace mpc
