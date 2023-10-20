#pragma once

#include <cassert>

#include <immer/flex_vector.hpp>
#include <immer/set.hpp>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <vector>

#include "../mpcgraph/builtin.hpp"
#include "common.hpp"
#include "node.hpp"
#include "operation.hpp"
namespace mpc {

/**
 * @brief This is a *persistent* graph class, which means that it is immutable and all manipulations will return a new graph.
 * GraphBase is unaware of corrupted parties, GraphBase is aware of corrupted parties.
 *
 */
class GraphBase {
    uint64_t generateHash(immer::flex_vector<OpVec> inEdgesOf) {
        uint64_t res = 0;
        for (OpVec inEdges: inEdgesOf) {
            uint64_t cur_res = 0;
            for (std::shared_ptr<Operation> edge: inEdges)
                cur_res ^= edge->hash;
            res = (res * 1000000009) + cur_res; // 23 is just some magic number
        }
        if (res == 0) {
            for (size_t i = 0; i < inEdgesOf.size(); ++i) {
                uint64_t cur_res = 0;
                for (std::shared_ptr<Operation> edge: inEdgesOf[i])
                    cur_res ^= edge->hash;
            }
        }
        return res;
    }

public:
    int guid = 0;

    // Since we need to transform a graph, we use a persistent adjacent list to represent a graph.
    // Also for efficiency, we do not directly use Node but only its id as index.
    immer::flex_vector<OpVec> inEdgesOf, outEdgesOf;
    NodeVec nodes;

    // Assuming the inEdgesOf and outEdgesOf are correctly synchronized, we only consider inEdgesOf for hashing.
    uint64_t hash = 0;

    GraphBase() {}
    GraphBase(const GraphBase& g):
        guid(g.guid), inEdgesOf(g.inEdgesOf), outEdgesOf(g.outEdgesOf), nodes(g.nodes), hash(g.hash) {}
    GraphBase(
        immer::flex_vector<OpVec> inEdgesOf,
        immer::flex_vector<OpVec> outEdgesOf,
        immer::flex_vector<std::shared_ptr<Node>> nodes
    ) :
        inEdgesOf(inEdgesOf), outEdgesOf(outEdgesOf), nodes(nodes), hash(generateHash(inEdgesOf)) {}
    // 移动构造函数
    GraphBase(GraphBase&& g):
        guid(std::move(g.guid)), inEdgesOf(std::move(g.inEdgesOf)), outEdgesOf(std::move(g.outEdgesOf)), nodes(std::move(g.nodes)), hash(g.hash) {}

    ~GraphBase() {}

    // 移动赋值运算符
    GraphBase& operator=(GraphBase&& g) noexcept {
        if (this != &g) {
            guid = std::move(g.guid);
            inEdgesOf = std::move(g.inEdgesOf);
            outEdgesOf = std::move(g.outEdgesOf);
            nodes = std::move(g.nodes);
            hash = std::move(g.hash);
        }
        return *this;
    }

    inline bool isRandomNode(std::shared_ptr<Node> node) const {
        assert(node->guid < (int)nodes.size());
        return inEdgesOf[node->guid].size() == 0 && !node->isInput() && !node->isOutput();
    }
    inline bool isRandomNode(size_t node_id) const {
        assert(node_id < nodes.size());
        return inEdgesOf[node_id].size() == 0 && !nodes[node_id]->isInput() && !nodes[node_id]->isOutput();
    }

    inline size_t inDeg(size_t node_id) const {
        assert(node_id < nodes.size());
        return inEdgesOf[node_id].size();
    }
    inline size_t inDeg(std::shared_ptr<Node> node) const {
        assert(node->guid < (int)nodes.size());
        return inEdgesOf[node->guid].size();
    }
    inline size_t outDeg(size_t node_id) const {
        assert(node_id < nodes.size());
        return outEdgesOf[node_id].size();
    }
    inline size_t outDeg(std::shared_ptr<Node> node) const {
        assert(node->guid < (int)nodes.size());
        return outEdgesOf[node->guid].size();
    }

    inline size_t nodeSize() const {
        return nodes.size();
    }
    inline size_t edgeSize() const {
        size_t edge_size = 0;
        for (size_t i = 0; i < nodes.size(); ++i) {
            edge_size += outEdgesOf[i].size();
        }
        return edge_size;
    }

    std::string to_string() const {
        std::stringstream ss;
        for (size_t node_id = 0; node_id < nodes.size(); ++node_id)
            if (nodes[node_id] != nullptr) {
                ss << nodes[node_id]->to_string() << std::endl;
                for (auto edge: inEdgesOf[node_id])
                    ss << "\t" << edge->to_string() << std::endl;
            }
        return ss.str();
    }

    friend std::ostream& operator<<(std::ostream& o, const GraphBase& g) {
        o << g.to_string();
        return o;
    }
};

/**
 * @brief GraphBase is unaware of corrupted parties, Graph is aware of corrupted parties. Also, there are some information (e.g. randomNodeCnt) to help the proving process are stored in Graph.
 */
class Graph: public GraphBase {
private:
    // assisting information
    unsigned _randomNodeCnt;
    unsigned _bubbleCnt;

public:
    // meta data
    unsigned partyCnt;
    unsigned T;
    immer::set<PartyDecl*> corruptedParties;

    Graph() {}
    Graph(const GraphBase& g,
        unsigned partyCnt, unsigned T, immer::set<PartyDecl*> corruptedParties,
        unsigned randomNodeCnt, unsigned bubbleCnt):
        GraphBase(g),
        _randomNodeCnt(randomNodeCnt), _bubbleCnt(bubbleCnt),
        partyCnt(partyCnt), T(T), corruptedParties(corruptedParties) {}
    // assisting informations are missed, then we could calculate them when initializing
    Graph(const GraphBase &g, unsigned partyCnt, unsigned T, immer::set<PartyDecl*> corruptedParties):
        GraphBase(g), partyCnt(partyCnt), T(T), corruptedParties(corruptedParties) {

        // compute randomNodeCnt
        _randomNodeCnt = 0;
        for (auto node: g.nodes)
            if (g.isRandomNode(node))
                ++_randomNodeCnt;

        // compute bubbleCnt
        _bubbleCnt = 0;
        for (auto node: g.nodes)
            if (corruptedParties.find(node->party) == nullptr) { // the node is at an honest party
                _bubbleCnt += node->isInput() || node->isOutput();
            } else { // the node is at a corrupted party
                _bubbleCnt += (node->isInput() || node->isOutput()) && g.inDeg(node) != 0;
            }
    }
    Graph(const Graph &g):
        GraphBase((GraphBase)g),
        _randomNodeCnt{g._randomNodeCnt}, _bubbleCnt(g._bubbleCnt),
        partyCnt(g.partyCnt), T(g.T), corruptedParties(g.corruptedParties) {}
    // 移动构造函数
    Graph(Graph&& g):
        GraphBase((GraphBase)g),
        _randomNodeCnt(g._randomNodeCnt), _bubbleCnt(g._bubbleCnt),
        partyCnt(g.partyCnt), T(g.T), corruptedParties(g.corruptedParties) {}

    ~Graph() {}

    bool operator== (const Graph& other) const {
        if (hash != other.hash) return false;

        if (_randomNodeCnt != other._randomNodeCnt) return false;
        if (_bubbleCnt != other._bubbleCnt) return false;

        // For efficiency, we don't have a sound equivalence checking here.

        return true;
    }

    // 移动赋值运算符
    Graph& operator=(Graph&& g) noexcept {
        if (this != &g) {
            guid = std::move(g.guid);
            hash = std::move(g.hash);
            inEdgesOf = std::move(g.inEdgesOf);
            outEdgesOf = std::move(g.outEdgesOf);
            nodes = std::move(g.nodes);
            _randomNodeCnt = std::move(g._randomNodeCnt);
            _bubbleCnt = std::move(g._bubbleCnt);
            partyCnt = std::move(g.partyCnt);
            T = std::move(g.T);
            corruptedParties = std::move(g.corruptedParties);
        }
        return *this;
    }

    Graph eliminateEdge(std::shared_ptr<Operation> edge) const {
        // 在 dst 的入边列表中把 edge 删掉
        auto dst = edge->getOutput();
        auto inEdgesOf = this->inEdgesOf;
        for (size_t id = 0; id < inEdgesOf[dst->guid].size(); ++id)
            if (inEdgesOf[dst->guid][id] == edge) {
                inEdgesOf = inEdgesOf.set(dst->guid, inEdgesOf[dst->guid].erase(id));
                break;
            }

        // 在 srcs 的出边列表中的 edge 删掉
        auto outEdgesOf = this->outEdgesOf;
        for (auto src: edge->getInputs()) {
            for (size_t id = 0; id < outEdgesOf[src->guid].size(); ++id)
                if (outEdgesOf[src->guid][id] == edge) {
                    outEdgesOf = outEdgesOf.set(src->guid, outEdgesOf[src->guid].erase(id));
                    break;
                }
        }

        // 计算 random nodes 的数量改变，删边之后是可能导致原来的 dst 变成 random node 的
        int addedRandomNodesCnt = inDeg(dst) == 1 && !dst->isInput() && !dst->isOutput();
        // 计算 bubble 的数量改变，删边之后是可能导致原来的 dst 由 bubble 变成不是 bubble 的。
        int decreasedBubbles = inDeg(dst) == 1 && (dst->isInput() || dst->isOutput()) && corruptedParties.find(dst->party) != nullptr;

        return Graph(
            GraphBase(inEdgesOf, outEdgesOf, nodes),
            partyCnt,
            T,
            corruptedParties,
            randomNodeCnt() + addedRandomNodesCnt,
            bubbleCnt() - decreasedBubbles
        );
    }
    Graph eliminateEdges(const std::vector<std::shared_ptr<Operation>>& edges) const {
        // 这个函数其实反复应用上面那个 eliminateEdge
        Graph g(*this);
        for (auto edge: edges)
            g = g.eliminateEdge(edge);
        return g;
    }
    Graph eliminateEdges(const immer::flex_vector<std::shared_ptr<Operation>>& edges) const {
        // 跟上面那个函数其实是一样的，只是把 std::vector 换成了 immer::flex_vector
        Graph g(*this);
        for (auto edge: edges)
            g = g.eliminateEdge(edge);
        return g;
    }
    Graph addEdge(std::shared_ptr<Operation> edge) const {
        // 在 dst 的入边列表里加入 edge
        auto dst = edge->getOutput();
        auto inEdgesOf = this->inEdgesOf;
        inEdgesOf = inEdgesOf.set(dst->guid, inEdgesOf[dst->guid].push_back(edge));

        // 在 srcs 的出边列表里加入 edge
        auto outEdgesOf = this->outEdgesOf;
        for (auto src: edge->getInputs()) {
            outEdgesOf = outEdgesOf.set(src->guid, outEdgesOf[src->guid].push_back(edge));
        }

        // 计算 random nodes 的数量的改变，加边之后可能导致原先是 random node 的 dst 变得不再是了
        int eliminatedRandomNodesCnt = inDeg(dst) == 0 && !dst->isInput() && !dst->isOutput();

        // 计算 bubble 数量的改变，如果 dst 是 corrupted party 的 input 或者 output 的话，我们要将其视为 bubble
        int addedBubbleCnt = inDeg(dst) == 0 && (dst->isInput() || dst->isOutput()) && corruptedParties.find(dst->party) != nullptr;

        return Graph(
            GraphBase(inEdgesOf, outEdgesOf, nodes),
            partyCnt,
            T,
            corruptedParties,
            randomNodeCnt() - eliminatedRandomNodesCnt,
            bubbleCnt() + addedBubbleCnt
        );
    }
    Graph addEdges(const std::vector<std::shared_ptr<Operation>>& edges) const {
        // 这个函数其实反复应用上面那个 addEdge
        Graph g(*this);
        for (auto edge: edges)
            g = g.addEdge(edge);
        return g;
    }
    Graph addEdges(const immer::flex_vector<std::shared_ptr<Operation>>& edges) const {
        // 跟上面那个函数其实是一样的，只是把参数里的 std::vector 换成了 immer::flex_vector
        Graph g(*this);
        for (auto edge: edges)
            g = g.addEdge(edge);
        return g;
    }
    Graph eliminateNode(std::shared_ptr<Node> node) const {
        auto inEdgesOf = this->inEdgesOf.set(node->guid, OpVec()); // 删掉 node 的入边
        auto outEdgesOf = this->outEdgesOf.set(node->guid, OpVec()); // 删掉 node 的出边

        // 在别人的入边列表里删去从 node 出发的边
        for (auto outEdge: this->outEdgesOf[node->guid]) {
            auto dst = outEdge->getOutput();
            for (size_t id = 0; id < inEdgesOf[dst->guid].size(); ++id)
                if (inEdgesOf[dst->guid][id] == outEdge) {
                    inEdgesOf = inEdgesOf.set(dst->guid, inEdgesOf.at(dst->guid).erase(id));
                    break;
                }
        }
        // 在别人的出边列表里删去到达 node 的边
        for (auto inEdge: this->inEdgesOf[node->guid]) {
            for (auto src: inEdge->getInputs()) {
                for (size_t id = 0; id < outEdgesOf[src->guid].size(); ++id)
                    if (outEdgesOf[src->guid][id] == inEdge) {
                        outEdgesOf = outEdgesOf.set(src->guid, outEdgesOf.at(src->guid).erase(id));
                        break;
                    }
            }
        }

        return Graph(
            GraphBase(
                inEdgesOf,
                outEdgesOf,
                nodes.set(node->guid, nullptr) // 在节点列表 nodes 中删掉 node
            ),
            partyCnt,
            T,
            corruptedParties,
            randomNodeCnt(),
            bubbleCnt() - ((node->isInput() || node->isOutput()) && corruptedParties.find(node->party) == nullptr) // 如果被删掉的节点是 honest party 的 input 或者 output 的话，bubble 数量 - 1
        );
    }

    int randomNodeCnt() const {
        return _randomNodeCnt;
    }
    int bubbleCnt() const {
        return _bubbleCnt;
    }
    bool hasBubble() const {
        return bubbleCnt() > 0;
    }

    Potential potential() const {
        // init search
        std::vector<bool> visited(nodeSize(), false);
        std::queue<size_t> q;
        // second term: the honest parties' nodes that are reachable to the corrupted parties
        // let's start from the corrupted parties' nodes
        for (size_t node_id = 0; node_id < nodeSize(); ++node_id)
            if (nodes[node_id] != nullptr &&
                corruptedParties.find(nodes[node_id]->party) != nullptr) // the node is at a corrupted party
                q.push(node_id);
        // do search & calculate the second term
        int numReachableNodes = 0;
        while (!q.empty()) {
            size_t node_id = q.front();
            q.pop();
            if (visited[node_id]) continue;
            visited[node_id] = true;
            if (corruptedParties.find(nodes[node_id]->party) == nullptr) {
                // the node is at an honest party
                ++numReachableNodes;
            }
            for (auto e : inEdgesOf[node_id]) {
                for (auto v : e->getInputs()) {
                    q.push(v->guid);
                }
            }
        }
        return Potential{bubbleCnt(), numReachableNodes, nodeSize()};
    }

    struct Hash
    {
        uint64_t operator() (const Graph& graph) const
        {
        return graph.hash;
        }
    };

    // old version, update required
    // std::string getHashString() {
    //     std::string hashstring_graph;
    //     // edge
    //     hashstring_graph += "EDGE";
    //     std::vector<std::string> string_edges;
    //     for (auto edge : edges) {
    //         if (edge != nullptr && !edge->isEliminated()) {
    //             std::string hashstring_edge;
    //             std::vector<std::string> string_inputs, string_outputs;
    //             for (auto input : edge->getInputs()) {
    //                 if (input != nullptr && !input->isEliminated()) {
    //                     string_inputs.push_back(input->name);
    //                 }
    //             }
    //             if (edge->getOutput() != nullptr &&
    //                 !edge->getOutput()->isEliminated()) {
    //                 string_outputs.push_back(edge->getOutput()->name);
    //             }
    //             std::sort(string_inputs.begin(), string_inputs.end());
    //             std::sort(string_outputs.begin(), string_outputs.end());
    //             std::string hashstring_input = "_INPUT_",
    //                 hashstring_output = "_OUTPUT_";
    //             for (auto string_input : string_inputs) {
    //                 hashstring_input += string_input;
    //             }
    //             for (auto string_output : string_outputs) {
    //                 hashstring_output += string_output;
    //             }
    //             hashstring_edge =
    //                 hashstring_input + hashstring_output +
    //                 std::to_string(static_cast<unsigned int>(edge->getType()));
    //             string_edges.push_back(hashstring_edge);
    //         }
    //     }
    //     std::sort(string_edges.begin(), string_edges.end());
    //     for (auto string_edge : string_edges) {
    //         hashstring_graph += string_edge;
    //     }
    //     // node
    //     hashstring_graph += "_NODE_";
    //     for (auto node : nodes) {
    //         if (node != nullptr && !node->isEliminated()) {
    //             hashstring_graph += node->name;
    //         }
    //     }
    //     return hashstring_graph;
    // }
};
}  // end of namespace mpc
