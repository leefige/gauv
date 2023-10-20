#pragma once

#include <cassert>

#include <spdlog/spdlog.h>

#include <vector>
#include <unordered_set>
#include <utility>
#include <memory>

#include "graph.hpp"
#include "node.hpp"
#include "operation.hpp"

namespace mpc {
/**
 * @brief 这里我们目前采取这样的实现：有一个 Transformer 基类，对于每条 production，我们新建一个子类。
 * 
 */
class Transformer {
public:
    typedef std::pair<std::shared_ptr<Node>, Graph> ResultType;
    typedef std::vector<std::pair<std::shared_ptr<Node>, Graph>> ResultsType;

    // some utils

    /**
     * @brief 节点 node_id 在图 g 中是否是一个"secret"
     * 
     * @param g 
     * @param node_id
     * @param srcParties 被送到这些 party 里的 share，我们会检查其是否入度为 1，也就是其是否可以被转成一个 random node
     * @return true 
     * @return false 
     */
    bool isRewritableSecret(const Graph &g, int node_id, std::unordered_set<PartyDecl*> srcParties) {
        assert(node_id < (int)g.nodes.size());
        assert(g.T < g.partyCnt); // T + 1 <= N

        if (g.outEdgesOf[node_id].size() != g.partyCnt) // 是否恰好有 N 条出边
            return false;
        std::unordered_set<int> src_ids;
        std::unordered_set<PartyDecl*> parties;

        for (auto edge : g.outEdgesOf[node_id]) {
            if (edge->getInputs().size() != g.T + 1) // 是否每条出边恰好有 T + 1 个源点
                return false;
            for (auto src: edge->getInputs()) {
                src_ids.insert(src->guid);
            }
            if (edge->getType() == Operator::EVAL) // 是否每条出边的类型都是 EVAL
            {
                std::shared_ptr<Node> share = edge->getOutput();
                if (g.outEdgesOf[share->guid].size() == 1) {
                    auto transit_edge = g.outEdgesOf[share->guid][0];
                    if (transit_edge->getType() == Operator::TRANSIT) { // 这条出边的类型是 TRANSIT
                        share = transit_edge->getOutput(); // 我们改成考虑 transit 之后的 share
                    }
                }

                PartyDecl* party = share->party;
                if (parties.contains(party)) { // 确保每条出边的目标节点都是不同的 party
                    return false;
                }
                parties.insert(party);

                if (srcParties.contains(party)) {
                    assert(g.inDeg(share) >= 1);
                    if (g.inDeg(share) > 1) // 如果入度不为 1 的话，之后就不能把它转为 random node 了！
                        return false;
                }
            }
        }
        if (src_ids.size() != g.T + 1) // 是否每条出边的 T + 1 个源点是一样的
            return false;
        assert(src_ids.contains(node_id)); // 这 T + 1 个源点要包含 node 自己
        for (auto src_id: src_ids)
            if (src_id != node_id && !g.isRandomNode(src_id)) // 剩下的 T 个源点应该是 random nodes
                return false;
        
        // 如果所有检查都通过了的话，就返回 true
        return true;
    }

    // 输入一个图，然后返回用这个 transformer 作变换能得到的所有可能的图的列表
    virtual ResultsType apply(const Graph &g) = 0;

    virtual std::string to_string() {
        return "Transformer";
    }
};

// Heuristics：
//      TODO: 已经生成的边就不再被纳入考虑，我们不会再被 rewrite
//      TODO: 最后再统一作 tail elimination
//      TODO: 用张为和王拓为提出的这个 SearchSet 的方法

// TODO: 其实目前的 subgraph matching 的实现还是相当 ad-hoc 的，如果有机会的话，将来最好是能搞一个更加 general 的实现。

/**
 * @brief c = a + b =====> a = c - b or b = c - a
 * 
 */
class AdditionTransformer : public Transformer {
public:
    virtual ResultsType apply(const Graph &g) override {
        ResultsType results; // 这个是用来保存所有可能的 transformation 的结果的
        for (int node_id = 0; node_id < (int)g.nodeSize(); ++node_id)
            for (auto edge: g.inEdgesOf[node_id]) {
                if (edge->getType() == Operator::ADD) {
                    assert(edge->getInputs().size() == 2);

                    // 删掉旧边
                    Graph g_eliminated = g.eliminateEdge(edge);

                    // 加入新边
                    std::shared_ptr<Node> src0 = edge->getInputs()[0], src1 = edge->getInputs()[1];
                    std::shared_ptr<Node> dst = edge->getOutput();
                    assert(dst == g.nodes[node_id]);

                    // case 0: src0 = dst - src1
                    auto new_edge0 = std::make_shared<Operation>(
                        Operator::SUB,
                        NodeVec{dst, src1},
                        src0
                    );
                    Graph g0 = g_eliminated.addEdge(new_edge0);
                    results.push_back(std::make_pair(dst, g0));

                    // case 1: src1 = dst - src0
                    auto new_edge1 = std::make_shared<Operation>(
                        Operator::SUB,
                        NodeVec{dst, src0},
                        src1
                    );
                    Graph g1 = g_eliminated.addEdge(new_edge1);

                    results.push_back(std::make_pair(dst, g1));
                }
            }
        ResultsType filtered_results;
        for (ResultType result: results)
            if (result.second.randomNodeCnt() == g.randomNodeCnt())
                filtered_results.push_back(std::move(result));
        return filtered_results;
    }

    virtual std::string to_string() override {
        return "REVERSE_ADDITION";
    }
};

/**
 * @brief evaluate n shares from secret with t random nodes ====> use the secret and t random shares to evaluate the others.
 * 
 */
class SharingTransformer : public Transformer {
    std::unordered_set<PartyDecl*> srcParties;

public:
    SharingTransformer(std::unordered_set<PartyDecl*> srcParties) : srcParties(srcParties) {
        spdlog::trace("srcParties for sharing transformer:");
        for (auto party: srcParties)
            spdlog::trace("\t{}", party->to_string());
    }

    virtual ResultsType apply(const Graph &g) override {
        ResultsType results; // 这个是用来保存所有可能的 transformation 的结果的
        for (int node_id = 0; node_id < (int)g.nodes.size(); ++node_id) {
            // TODO: maybe this loop could be optimized by only enumerating a "potential nodes" list.

            // 检查它是否是 random node
            // if (g.isRandomNode(node_id)) continue;

            // 检查 node_id 是否是那个用来生成整个 Shamir sharing 的 “secret”
            if (!isRewritableSecret(g, node_id, srcParties)) continue;

            // 收集旧边
            std::vector<std::shared_ptr<Operation>> edges_to_eliminate;
            std::vector<std::shared_ptr<Operation>> edges_to_add;
            std::vector<std::shared_ptr<Node>> srcs { g.nodes[node_id] };
            std::unordered_set<std::shared_ptr<Node>> dsts;
            for (auto edge: g.outEdgesOf[node_id]) {
                assert(edge->getType() == Operator::EVAL);
                edges_to_eliminate.push_back(edge);

                // 收集表示随机系数的节点，之后会作为 EVAL 的 destination
                for (auto src: edge->getInputs())
                    if (src->guid != node_id)
                        dsts.insert(src);

                auto share = edge->getOutput();
                if (g.outEdgesOf[share->guid].size() == 1
                    && g.outEdgesOf[share->guid][0]->getType() == Operator::TRANSIT) {
                    // 这个 share 被 transit 到了另一个 party
                    std::shared_ptr<Node> transit_share = g.outEdgesOf[share->guid][0]->getOutput();

                    if (srcParties.contains(transit_share->party)) {
                        edges_to_eliminate.push_back(g.outEdgesOf[share->guid][0]);
                        edges_to_add.push_back(std::make_shared<Operation>(
                            Operator::TRANSIT,
                            NodeVec{transit_share},
                            share
                        ));
                        srcs.push_back(share);
                    } else {
                        dsts.insert(share);
                    }
                } else {
                    if (srcParties.contains(share->party)) {
                        srcs.push_back(share);
                    } else {
                        dsts.insert(share);
                    }
                }
            }
            // 建新的 EVAL 边
            assert(dsts.size() == g.partyCnt);
            for (auto dst: dsts) {
                auto new_edge = make_shared<Operation>(Operator::EVAL, srcs, dst);
                edges_to_add.push_back(new_edge);
            }
            // 建图
            Graph g_new = g.eliminateEdges(edges_to_eliminate).addEdges(edges_to_add);
            results.push_back(std::make_pair(g.nodes[node_id], g_new));
        }
        ResultsType filtered_results;
        for (ResultType result: results)
            if (result.second.randomNodeCnt() == g.randomNodeCnt())
                filtered_results.push_back(std::move(result));
        return filtered_results;
    }

    virtual std::string to_string() override {
        return "REVERSE_SHARING";
    }
};

/**
 * @brief evaluate n shares with a random secret and t random nodes ====> use t + 1 random shares to evaluate the others
 * 
 */
class RandomSharingTransformer : public Transformer {
    std::unordered_set<PartyDecl*> srcParties;

public:
    RandomSharingTransformer(std::unordered_set<PartyDecl*> srcParties) : srcParties(srcParties) {
        spdlog::trace("srcParties of random sharing transformer are:");
        for (auto party: srcParties)
            spdlog::trace("\t{}", party->to_string());
    }

    virtual ResultsType apply(const Graph& g) override {
        ResultsType results; // 这个是用来保存所有可能的 transformation 的结果的
        for (int node_id = 0; node_id < (int)g.nodes.size(); ++node_id) {
            // 检查它是否是 random node
            if (!g.isRandomNode(node_id)) continue;

            // 以下内容其实基本是从上面的 SharingTransformer 复制过来的qwq
            // 检查 node_id 是否是那个用来生成整个 Shamir sharing 的 “secret”
            if (!isRewritableSecret(g, node_id, srcParties)) continue;

            // 收集旧边
            std::vector<std::shared_ptr<Operation>> edges_to_eliminate;
            std::vector<std::shared_ptr<Operation>> edges_to_add;
            std::vector<std::shared_ptr<Node>> srcs;
            std::unordered_set<std::shared_ptr<Node>> dsts { g.nodes[node_id] };
            for (auto edge: g.outEdgesOf[node_id]) {
                assert(edge->getType() == Operator::EVAL);
                edges_to_eliminate.push_back(edge);

                // 收集表示随机系数的节点，之后会作为 EVAL 的 destination
                for (auto src: edge->getInputs())
                    if (src->guid != node_id)
                        dsts.insert(src);

                auto share = edge->getOutput();
                if (g.outEdgesOf[share->guid].size() == 1
                    && g.outEdgesOf[share->guid][0]->getType() == Operator::TRANSIT) {
                    // 这个 share 被 transit 到了另一个 party
                    auto transit_share = g.outEdgesOf[share->guid][0]->getOutput();

                    if (srcParties.contains(transit_share->party)) {
                        edges_to_eliminate.push_back(g.outEdgesOf[share->guid][0]);
                        edges_to_add.push_back(std::make_shared<Operation>(
                            Operator::TRANSIT,
                            NodeVec{transit_share},
                            share
                        ));
                        srcs.push_back(share);
                    } else {
                        dsts.insert(share);
                    }
                } else {
                    if (srcParties.contains(share->party)) {
                        srcs.push_back(share);
                    } else {
                        dsts.insert(share);
                    }
                }
            }
            // 建新的 EVAL 边
            assert(dsts.size() == g.partyCnt);
            for (auto dst: dsts) {
                auto new_edge = std::make_shared<Operation>(Operator::EVAL, srcs, dst);
                edges_to_add.push_back(new_edge);
            }
            // 建图
            Graph g_new = g.eliminateEdges(edges_to_eliminate).addEdges(edges_to_add);
            results.push_back(std::make_pair(g.nodes[node_id], g_new));
        }
        ResultsType filtered_results;
        for (ResultType result: results)
            if (result.second.randomNodeCnt() == g.randomNodeCnt())
                filtered_results.push_back(std::move(result));
        return filtered_results;
    }

    virtual std::string to_string() override {
        return "REVERSE_RANDOM_SHARING";
    }
};

/**
 * @brief reconstruct a secret from n shares ====> use the secret and t shares to evaluate other shares
 * 
 */
class ReconstructionTransformer : public Transformer {
    std::unordered_set<PartyDecl*> srcParties;

    bool isRewritableReconstruction(const Graph& g, std::shared_ptr<Operation> edge) {
        assert(edge->getInputs().size() == g.partyCnt); // 应该恰好是把 N 个 share 收集起来做 reconstruction
        std::unordered_set<PartyDecl*> parties;
        int random_dst_cnt = 0;
        for (auto share: edge->getInputs()) {
            if (g.inEdgesOf[share->guid].size() == 1) {
                auto transit_edge = g.inEdgesOf[share->guid][0];
                if (transit_edge->getType() == Operator::TRANSIT) { // 这条出边的类型是 TRANSIT)
                    assert(transit_edge->getInputs().size() == 1);
                    share = transit_edge->getInputs()[0]; // 我们改成考虑 transit 之后的 share
                }
            }
            
            if (parties.contains(share->party)) // 每个 share 是来自不同的 party
                return false;
            parties.insert(share->party);

            if (!srcParties.contains(share->party)) {
                random_dst_cnt += g.isRandomNode(share);
            }
        }
        int whether_secret_will_be_random = g.inDeg(edge->getOutput()) == 1 && !edge->getOutput()->isInput() && !edge->getOutput()->isOutput();
        if (random_dst_cnt != whether_secret_will_be_random) // 期望 destination share 中的 random node 的数量应该是和 secret 是否会是 random 的数量是一致的
            return false;
        
        // 如果所有检查都通过了的话，就返回 true
        return true;
    }
public:
    ReconstructionTransformer(std::unordered_set<PartyDecl*> srcParties)
        : srcParties(srcParties) {
            spdlog::trace("srcParties for reconstruction transformer:");
            for (auto party: srcParties)
                spdlog::trace("\t{}", party->to_string());
        }

    virtual ResultsType apply(const Graph& g) override {
        ResultsType results; // 这个是用来保存所有可能的 transformation 的结果的
        for (int node_id = 0; node_id < (int)g.nodes.size(); ++node_id)
            if (g.inEdgesOf[node_id].size() == 1 && g.inEdgesOf[node_id][0]->getType() == Operator::RECONSTRUCT) {
                auto reconstruction_edge = g.inEdgesOf[node_id][0];

                // 首先我们来检查 pattern
                if (!isRewritableReconstruction(g, reconstruction_edge)) continue;
                
                // 收集旧边
                std::vector<std::shared_ptr<Operation>> edges_to_eliminate;
                std::vector<std::shared_ptr<Operation>> edges_to_add;
                std::vector<std::shared_ptr<Node>> srcs;
                std::unordered_set<std::shared_ptr<Node>> dsts;
                srcs.push_back(g.nodes[node_id]);
                edges_to_eliminate.push_back(reconstruction_edge);
                for (auto share: reconstruction_edge->getInputs()) {
                    if (g.inEdgesOf[share->guid].size() == 1 && g.inEdgesOf[share->guid][0]->getType() == Operator::TRANSIT) { // 这个 share 是 transit 过来的
                        auto transit_edge = g.inEdgesOf[share->guid][0];
                        assert(transit_edge->getInputs().size() == 1);
                        auto share_before = transit_edge->getInputs()[0]; // 我们改成考虑 transit 之后的 share

                        if (srcParties.contains(share_before->party)) {
                            srcs.push_back(share);
                        } else {
                            dsts.insert(share);

                            edges_to_eliminate.push_back(transit_edge);
                            edges_to_add.push_back(std::make_shared<Operation>(
                                Operator::TRANSIT,
                                NodeVec{share},
                                share_before
                            ));
                        }
                    } else { // 这个 share 不是 transit 过来的
                        if (srcParties.contains(share->party)) {
                            srcs.push_back(share);
                        } else {
                            dsts.insert(share);
                        }
                    }
                }
                // 建新的 EVAL 边
                for (auto dst: dsts) {
                    auto new_edge = std::make_shared<Operation>(
                        Operator::EVAL,
                        NodeVec(srcs.begin(), srcs.end()),
                        dst);
                    edges_to_add.push_back(new_edge);
                }
                // 建图
                // 一条必须删的边是 reconstruction 这条边，然后我们可能会取 t + 1 个 share，还剩下 n - t - 1 个 share 里可能还有一个不需要 transit 的，所以需要 transit 的就至少是 n - t - 2 个，那么总计就是至少要删 n - t - 1 条边。
                assert((int)edges_to_eliminate.size() >= (int)(g.partyCnt - g.T - 1));
                // 最多会有 n - t - 1 个 share 作为 dst，然后里面还可能有一个不需要 transit 的，所以加的边最少是 2n - 2t - 3 条
                assert((int)edges_to_add.size() >= (int)(2 * (g.partyCnt - g.T - 1) - 1));
                Graph g_new = g.eliminateEdges(edges_to_eliminate).addEdges(edges_to_add);

                results.push_back(std::make_pair(g.nodes[node_id], g_new));
            }
        ResultsType filtered_results;
        for (ResultType result: results)
            if (result.second.randomNodeCnt() == g.randomNodeCnt())
                filtered_results.push_back(std::move(result));
        return filtered_results;
    }

    virtual std::string to_string() override {
        return "REVERSE_RECONSTRUCTION";
    }
};

// class TailNodeTransformer : public Transformer {
// public:
//     virtual vector<pair<Node*, Graph>> apply(const Graph& g) override {
//         vector<pair<Node*, Graph>> results; // 这个是用来保存所有可能的 transformation 的结果的
//         for (auto node: g.nodes) {
//             if (g.outDeg(node) == 0) {
//                 graphs.push_back(g.eliminateNode(node));
//             }
//         }
//         return results;
//     }
// }
} // end of namespace mpc