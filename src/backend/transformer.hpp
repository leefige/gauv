#pragma once

#include <assert.h>

#include <vector>
#include <unordered_set>

#include "graph.hpp"
#include "node.hpp"

using namespace std;

// 这里我们目前采取这样的实现：
// 有一个 Transformer 基类，
// 对于每条 production，我们新建一个子类。
class Transformer {
private:
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
    bool isRewritableSecret(const Graph &g, int node_id, unordered_set<PartyDecl*> srcParties) {
        assert(node_id < g.nodes.size());
        assert(g.orruptedPartyCnt < g.partyCnt); // T + 1 <= N

        if (outEdgesOf[node_id] != g.partyCnt) // 是否恰好有 N 条出边
            return false;
        unordered_set<int> src_ids;
        unordered_set<PartyDecl*> parties;
        for (auto edge : outEdgesOf[node_id]) {
            if (edge->getInputs().size() != g.corruptedPartyCnt + 1) // 是否每条出边恰好有 T + 1 个源点
                return false;
            for (auto src: edge->getInputs()) {
                src_ids.insert(src->guid);)
            }
            if (edge->getType() == Operator::EVAL) // 是否每条出边的类型都是 EVAL
            {
                Node* share = edge->getOutput();
                if (g.outEdgesOf[share->guid].size() == 0) {
                    if (parties.contains(share->party)) // 是否每条出边的目标节点都是不同的 party
                        return false;
                    parties.insert(share->party);
                } if (g.outEdgesOf[share->guid].size() == 1) {
                    auto edge = g.outEdgesOf[share->guid][0];
                    if (edge->getType() != Operator::TRANSIT) // 这条出边的类型是 TRANSIT)
                        return false;
                    
                    auto transitted_share = edge->getOuput();
                    Node* party = transitted_share->party;
                    if (parties.contains(party)) // 是否每条出边的目标节点都是不同的 party
                        return false;
                    parties.insert(party);

                    if (srcParties.contains(party)) {
                        assert(g.inDeg(transitted_share) >= 1);
                        if (g.inDeg(transitted_share) > 1) // 如果入度不为 1 的话，之后就不能把它转为 random node 了！
                            return false;
                    }
                }
                else return false; // 每个 share 有不超过一条出边
            }
        }
        if (src_ids.size() != g.corruptedPartyCnt + 1) // 是否每条出边的 T + 1 个源点是一样的
            return false;
        assert(!srcs.contains(node_id)); // 这 T + 1 个源点要包含 node 自己
        for (auto src_id: src_ids)
            if (!g.isRandomNode(src_id)) // 剩下的 T 个源点应该是 random nodes
                return false;
        return true;
    }
public:
    // 输入一个图，然后返回用这个 transformer 作变换能得到的所有可能的图的列表
    virtual vector<Graph> apply(const Graph &g) const;
}

// Heuristics：
//      TODO: 已经生成的边就不再被纳入考虑，我们不会再被 rewrite

// TODO: 考虑 random nodes（虽然在文章里我们直接把 random node 定义成零出度的节点，但这里在实际实现的时候我们还是手动来维护哪些节点是 random nodes）

// TODO: 其实目前的 subgraph matching 的实现还是相当 ad-hoc 的，如果有机会的话，将来最好是能搞一个更加 general 的实现。

// c = a + b =====> a = c - b or b = c - a
class AdditionTransformer : public Transformer {
public:
    virtual vector<Graph> apply(const Graph &g) override {
        vector<Graph> graphs;
        for (int node_id = 0; node_id < g.nodeCnt; ++node_id)
            for (auto edge: inEdgesOf[node_id]) {
                if (edge->getType() == Operator::ADD) {
                    assert(edge->getInputs().size() == 2, "The input size of an addition edge must be 2");

                    // 删掉旧边
                    Graph g_eliminated = g.eliminateEdge(edge);

                    // 加入新边
                    Node *src0 = edge->getInputs()[0], *src1 = edge->getInputs()[1];
                    Node *dst = edge->getOutput();

                    // case 0: src0 = dst - src1
                    auto new_edge0 = make_shared<Operation>(Operator::SUB, {dst, src1}, src0);
                    Graph g0 = g_eliminated.addEdge(new_edge0);
                    graphs.push_back(g0);

                    // case 1: src1 = dst - src0
                    auto new_edge1 = make_shared<Operation>(Operator::SUB, {dst, src0}, src1);
                    Graph g1 = g_eliminated.addEdge(new_edge1);
                    graphs.push_back(g1);
                }
            }
        return graphs;
    }
}

// evaluate n shares from secret with t random nodes ====> use the secret and the first t random shares to evaluate the others
class InputSharingTransformer : public Transformer {
    unordered_set<PartyDecl*> srcParties;

public:
    virtual vector<Graph> apply(const Graph &g) override {
        vector<Graph> graphs;
        for (int node_id = 0; node_id < g.nodeCnt; ++node_id) {
            // 检查它是否是 input
            if (!g.nodes[node_id].isInput()) continue;

            // 检查 node_id 是否是那个用来生成整个 Shamir sharing 的 “secret”
            if (!isRewritableSecret(g, node_id, srcParties)) continue;
        }

        for (Operation* edge: g->edges()) {
            if (edge->getType()) {
                assert(edge->getInputs().size() == g->T + 1, "The input size of a polynomial edge must be T + 1");
                Node *secret = edge->inputs[0];
                if (secret->getType() != Node::Type::RANDOM) {
                    NodeVec srcs, dests;
                    srcs.push_back(secret);
                    OpVec old_edges;
                    // TODO: check whether all nodes come from a same polynomial
                    for (auto e : node->getOuputs()) {
                        if (!e->isEliminated() && !e->isGenerated() &&
                            e->getType() == Operator::EVAL) {
                            old_edges.push_back(e);
                            auto node = e->getOutput();
                            auto party = g->traceTargetParty(node);
                            if (srcParties.count(party)) {
                                srcs.push_back(node);
                            } else {
                                dests.push_back(node);
                            }
                        }
                    }
                    if (old_edges.empty()) return false;
                    if (srcs.size() != T + 1) return false;
                    for (auto d : srcs) {
                        if (d->isRandom()) return false;
                        if (d != secret && d->getValidInDegrees() != 1) // if not, the number of random nodes would change
                            return false;
                    }

                    for (auto e : old_edges) e->markEliminated();
                    for (auto v : old_edges.front()->getInputs())
                        if (v != secret)
                            dests.push_back(v);
                    for (auto v : srcs)
                        if (v != secret)
                            v->type = Node::RANDOM;
                    for (auto d : dests) {
                        if (d->isRandom()) d->type = Node::NONE;
                        auto edge = new Operation(Operator::EVAL, srcs, d);
                        edge->markGenerated();
                        for (auto v : srcs) v->addOutputOp(edge);
                        d->addInputOp(edge);
                        edges.push_back(edge);

                        if (!d->party->is_corrupted() && 1 < d->getValidInDegrees()) {
                            // 入度非零的 honest inputs 被算作 bubble
                            d->state = Node::BUBBLE;
                            bubbleSet.insert(d);
                        } else {
                            d->markPotential();
                        }
                        searchSet.insert(d);
                    }
                }
            }
        }
        return graphs;
    }
}

// evaluate n shares with a random secret and t random nodes ====> use the secret and the first t random shares to evaluate the others
class RandomSharingTransformer : public Transformer {
public:
    virtual vector<Graph> apply(const Graph& g) override {}
}

class ReconstructionTransformer : public Transformer {
public:
    virtual vector<Graph> apply(const Graph& g) override {}
}
