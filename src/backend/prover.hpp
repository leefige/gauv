#pragma once

#include <chrono>

#include <vector>
#include <unordered_set>

#include "graph.hpp"
#include "transformer.hpp"

namespace mpc {

class Prover {
    const GraphBase& graph_base;
    const std::vector<std::unordered_set<PartyDecl *>> equivalent_classes;
    const std::vector<PartyDecl *> parties;
    const int T;
    const int verbose_level

    enum TransformType {
        TAIL_NODE,
        TAIL_EDGE,
        REVERSE_RECONSTRUCT,
        REVERSE_ADDITION,
        REVERSE_TRANSIT,
        SIM_POLY,
    };
    struct HistEntry {
        Node* node;
        TransformType type;
        Potential potential;
    };

    static std::string to_string(TransformType type) {
        switch (type) {
        case TAIL_NODE:
            return "TAIL_NODE";
        case TAIL_EDGE:
            return "TAIL_EDGE";
        case REVERSE_RECONSTRUCT:
            return "REVERSE_RECONSTRUCT";
        case REVERSE_TRANSIT:
            return "REVERSE_TRANSIT";
        case SIM_POLY:
            return "SIM_POLY";
        default:
            throw std::runtime_error("Bad TransformType");
        }
    }

    std::vector<HistEntry> transformTape;

    // 俺们现在的算法是这样的，每次选一个让秩函数下降最多的 transformation 来做。
    bool tryProving(Graph g) {
        // 初始化
        transformTape.clear();

        // 定义 equivalent rewriters
        Transformer additionRewriter = AdditionTransformer();
        // 设置 source parties
        std::unordered_set<PartyDecl*> srcParties;
        for (auto party: parties) {
            if (party->is_corrupted()) {
                srcParties.insert(party);
            }
        }
        // 加入 honest party 作为 source party 直到到达阈值 T
        for (auto party: parties) {
            if (party->is_honest()) {
                srcParties.insert(party);
                if (srcParties.size() == T)
                    break;
            }
        }
        Transformer inputSharingRewriter = InputSharingTransformer(srcParties);
        Transformer reconstructionRewriter1 = ReconstructionTransformer(srcParties);

        // 再多加一个 honest party 作为 source party，使得我们有 T + 1 个 source parties
        Transformer randomSharingRewriter = RandomSharingTransformer(srcParties);
        Transformer reconstructionRewriter2 = ReconstructionTransformer(srcParties);

        std::vector<Transformer> equivalent_rewriters{
            additionRewriter,
            inputSharingRewriter,
            randomSharingRewriter,
            reconstructionRewriter1,
            reconstructionRewriter2
        }
        
        while (g.hasBubble()) {
            bool transformed = false;
            Node* min_node = nullptr;
            Graph min_new_g;
            for (auto& transformer : equivalent_rewriters) {
                for (auto& (node, new_g): transformer.apply(g)) {
                    if (min_node == nullptr || min_new_g.potential() < g.potential()) {
                        min_node = node;
                        min_new_g = new_g;
                        transformed = true;
                        break;
                    }
                }
            }
            if (!transformed) break;
        }

        while (true) {
            bool transformed = false;
            for (auto node : g.nodes)
                if (g.outDeg(node) == 0 && node->party->is_honest()) {
                    g = g.eliminateNode(node);
                    transformed = true;
                }
            if (!transformed) break;
        }

        return !g.hasBubble();
    }

    function<bool(const Graph&)> algorithm = tryProving; // 这样写是为了更灵活，将来可以方便地替换这个核心算法

    void search_all_possibilities(int equivalent_class_id, int corrupted_quota) {
        if (equivalent_class_id == equivalent_classes.size()) {
            if (corrupted_quota == 0) {
                Graph g(graph_base, parties.size(), T);

                auto begin_time = chrono::high_resolution_clock::now();
                bool proved = algorithm(g);
                auto end_time = chrono::high_resolution_clock::now();
                cout << endl << "Proved? " << std::boolalpha << proved << endl;
                cout << "Time: " << (end_time - begin_time).count() / 1e9 << endl;
                cout << "Final graph has " << graph.nodeSize() << " nodes, "
                    << graph.edgeSize() << " edges" << endl;
                cout << "Final potential: " << to_string(graph.potential()) << endl;
                if (verbose_level >= 3) cout << "Final graph:" << endl << graph << endl;
                if (verbose_level >= 2) {
                    cout << endl << "Transform history:" << endl;
                    for (auto& entry : transformTape) {
                        cout << entry.node->getName()
                            << " " << to_string(entry.type)
                            << " " << to_string(entry.potential) << endl;
                    }
                }
            }
            return;
        }
        for (int i = 0; i <= min(corrupted_quota, equivalent_classes[equivalent_class_id].size()); ++i) {
            // 在这个等价类里面选 i 个 corrupted_aprty
            int j = 0;
            for (auto party : equivalent_classes[equivalent_class_id]) {
                if (j < i)
                    party->set_corrupted();
                else
                    party->set_honest();
                
                ++j;
            }
            search_all_possibilities(equivalent_class_id + 1, corrupted_quota - i);
        }
    }
public:
    Prover(
        const GraphBase& graph_base,
        std::vector<std::unordered_set<PartyDecl *>> equivalent_classes,
        std::vector<std::unordered_set<PartyDecl *>> parties,
        const int T,
        const int verbose_level = 3
    ): graph_base(graph_base), equivalent_classes(equivalent_classes), parties(parties), T(T), verbose_level(verbose_level) {
        cout << "Initial graph has " << graph.nodeSize() << " nodes, "
            << graph.edgeSize() << " edges" << endl;
        cout << "Initial potential: " << GraphProver::to_string(graph.potential())
            << endl;
        if (verbose_level >= 3) cout << endl << "Initial graph:" << endl << graph << endl;
    }
    ~GraphProver() {}

    void prove(int I) {
        search_all_possibilities(0, I);
    }
    void prove() {
        for (int I = 1; I <= T; ++I)
            prove(g, equivalent_classes, T, I);
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