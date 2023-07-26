#pragma once

#include <assert.h>

#include <immer/flex_vector.hpp>

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
public:
    int guid = 0;

    // Since we need to transform a graph, we use a persistent adjacent list to represent a graph.
    // Also for efficiency, we do not directly use Node but only its id as index.
    immer::flex_vector<OpVec> inEdgesOf, outEdgesOf;
    NodeVec nodes;

    GraphBase() {}
    GraphBase(const GraphBase& g):
        guid(g.guid), inEdgesOf(g.inEdgesOf), outEdgesOf(g.outEdgesOf), nodes(g.nodes) {}
    GraphBase(
        immer::flex_vector<OpVec> inEdgesOf,
        immer::flex_vector<OpVec> outEdgesOf,
        immer::flex_vector<std::shared_ptr<Node>> nodes
    ):
        inEdgesOf(inEdgesOf), outEdgesOf(outEdgesOf), nodes(nodes) {}
    // 移动构造函数
    GraphBase(GraphBase&& g):
        guid(std::move(g.guid)), inEdgesOf(std::move(g.inEdgesOf)), outEdgesOf(std::move(g.outEdgesOf)), nodes(std::move(g.nodes)) {}

    ~GraphBase() {}

    // 移动赋值运算符
    GraphBase& operator=(GraphBase&& g) noexcept {
        if (this != &g) {
            guid = std::move(g.guid);
            inEdgesOf = std::move(g.inEdgesOf);
            outEdgesOf = std::move(g.outEdgesOf);
            nodes = std::move(g.nodes);
        }
        return *this;
    }

    inline bool isRandomNode(Node* node) const {
        assert(node->guid < nodes.size());
        return inEdgesOf[node->guid].size() == 0 && !node->isInput() && !node->isOutput();
    }
    inline bool isRandomNode(int node_id) const {
        assert(node_id < nodes.size());
        return inEdgesOf[node_id].size() == 0 && !nodes[node_id]->isInput() && !nodes[node_id]->isOutput();
    }

    inline int inDeg(int node_id) const {
        assert(node_id < nodes.size());
        return inEdgesOf[node_id].size();
    }
    inline int inDeg(std::shared_ptr<Node> node) const {
        assert(node->guid < nodes.size());
        return inEdgesOf[node->guid].size();
    }
    inline int outDeg(int node_id) const {
        assert(node_id < nodes.size());
        return outEdgesOf[node_id].size();
    }
    inline int outDeg(std::shared_ptr<Node> node) const {
        assert(node->guid < nodes.size());
        return outEdgesOf[node->guid].size();
    }

    inline int nodeSize() const {
        return nodes.size();
    }
    inline int edgeSize() const {
        int edge_size = 0;
        for (int i = 0; i < nodes.size(); ++i) {
            edge_size += outEdgesOf[i].size();
        }
        return edge_size;
    }

    // TODO: hash
    friend std::ostream& operator<<(std::ostream& o, const GraphBase& g) {
        for (int node_id = 0; node_id < g.nodes.size(); ++node_id) {
            o << std::endl << g.nodes[node_id]->to_string();
            for (auto edge: inEdgesOf[node_id])
                o << '\t' << edge << std::endl;
        }
        return o;
    }
};

/**
 * @brief GraphBase is unaware of corrupted parties, Graph is aware of corrupted parties. Also, there are some information (e.g. randomNodeCnt) to help the proving process are stored in Graph.
 */
class Graph: public GraphBase {
private:
    // assisting information
    int _randomNodeCnt;
    int _bubbleCnt;

public:
    // meta data
    int partyCnt;
    int T;

    Graph(const GraphBase& g, int partyCnt, int T, int randomNodeCnt, int bubbleCnt):
        GraphBase(g), partyCnt(partyCnt), T(T), _randomNodeCnt(randomNodeCnt), _bubbleCnt(bubbleCnt) {}
    // assisting informations are missed, then we could calculate them when initializing
    Graph(const GraphBase &g, int partyCnt, int T):
        GraphBase(g) {
        
        // compute randomNodeCnt
        _randomNodeCnt = 0;
        for (auto node: g.nodes)
            if (g.isRandomNode(node))
                ++_randomNodeCnt;
        
        // compute bubbleCnt
        _bubbleCnt = 0;
        for (auto node: g.nodes)
            if (node->party->is_honest()) {
                _bubbleCnt += node->isInput() || node->isOutput();
            } else {
                assert(node->party->is_corrupted());
                _bubbleCnt += (node->isInput() || node->isOutput()) && g.inDeg(node) != 0;
            }
    }
    Graph(const Graph &g):
        GraphBase((GraphBase)g), _randomNodeCnt{g._randomNodeCnt}, _bubbleCnt(g._bubbleCnt), partyCnt(g.partyCnt), T(g.T) {}
    // 移动构造函数
    Graph(Graph&& g):
        GraphBase(std::move((GraphBase)g)), _randomNodeCnt(std::move(g._randomNodeCnt)), _bubbleCnt(std::move(g._bubbleCnt)), partyCnt(std::move(g.partyCnt)), T(std::move(g.T)) {}
    
    ~Graph() {}

    // 移动赋值运算符
    Graph& operator=(Graph&& g) noexcept {
        if (this != &g) {
            guid = std::move(g.guid);
            inEdgesOf = std::move(g.inEdgesOf);
            outEdgesOf = std::move(g.outEdgesOf);
            nodes = std::move(g.nodes);
            _randomNodeCnt = std::move(g._randomNodeCnt);
            _bubbleCnt = std::move(g._bubbleCnt);
            partyCnt = std::move(g.partyCnt);
            T = std::move(g.T);
        }
        return *this;
    }

    Graph eliminateEdge(std::shared_ptr<Operation> edge) const {
        // 在 dst 的入边列表中把 edge 删掉
        auto dst = edge->getOutput();
        auto inEdgesOf = this->inEdgesOf;
        inEdgesOf[dst->guid] = std::move(OpVec());
        
        // 在 srcs 的出边列表中的 edge 删掉
        auto outEdgesOf = this->outEdgesOf;
        for (auto src: edge->getInputs()) {
            outEdgesOf[src] = std::move(OpVec());
        }

        // 计算 random nodes 的数量改变，删边之后是可能导致原来的 dst 变成 random node 的
        int addedRandomNodesCnt = inDeg(dst) == 1 && !dst->isInput() && !dst->isOutput();

        Graph g(
            GraphBase(inEdgesOf, outEdgesOf, nodes),
            partyCnt,
            T,
            randomNodeCnt() + addedRandomNodesCnt,
            bubbleCnt()
        );
    }
    Graph eliminateEdges(const std::vector<std::shared_ptr<Operation>>& edges) const {
        // 这个函数其实反复应用上面那个 eliminateEdge
        Graph g(*this);
        for (auto edge: edges)
            g = std::move(g.eliminateEdge(edge));
        return g;
    }
    Graph eliminateEdges(const immer::flex_vector<std::shared_ptr<Operation>>& edges) const {
        // 跟上面那个函数其实是一样的，只是把 std::vector 换成了 immer::flex_vector
        Graph g(*this);
        for (auto edge: edges)
            g = std::move(g.eliminateEdge(edge));
        return g;
    }
    Graph addEdge(std::shared_ptr<Operation> edge) const {
        // 在 dst 的入边列表里加入 edge
        auto dst = edge->getOutput();
        auto inEdgesOf = this->inEdgesOf;
        inEdgesOf.set(dst->guid, inEdgesOf[dst->guid].push_back(edge));

        // 在 srcs 的出边列表里加入 edge
        auto outEdgesOf = this->outEdgesOf;
        for (auto src: edge->getInputs()) {
            outEdgesOf.set(src->guid, outEdgesOf[src->guid].push_back(edge));
        }

        // 计算 random nodes 的数量的改变，加边之后可能导致原先是 random node 的 dst 变得不再是了
        int eliminatedRandomNodesCnt = inDeg(dst) == 0 && !dst->isInput() && !dst->isOutput();

        // 计算 bubble 数量的改变，如果 dst 是 corrupted party 的 input 或者 output 的话，我们要将其视为 bubble
        int addedBubbleCnt = dst->party->is_corrupted() && (dst->isInput() || dst->isOutput());

        return Graph(
            GraphBase(inEdgesOf, outEdgesOf, nodes),
            partyCnt,
            T,
            randomNodeCnt() - eliminatedRandomNodesCnt,
            bubbleCnt() + addedBubbleCnt
        );
    }
    Graph addEdges(const std::vector<std::shared_ptr<Operation>>& edges) const {
        // 这个函数其实反复应用上面那个 addEdge
        Graph g(*this);
        for (auto edge: edges)
            g = std::move(g.addEdge(edge));
        return g;
    }
    Graph addEdges(const immer::flex_vector<std::shared_ptr<Operation>>& edges) const {
        // 跟上面那个函数其实是一样的，只是把参数里的 std::vector 换成了 immer::flex_vector
        Graph g(*this);
        for (auto edge: edges)
            g = std::move(g.addEdge(edge));
        return g;
    }
    Graph eliminateNode(Node* node) const {
        Graph g = this->eliminateEdges(inEdgesOf[node->guid]); // 删掉 node 的入边
        g = g.eliminateEdges(outEdgesOf[node->guid]); // 删掉 node 的出边
        return Graph(
            GraphBase(g.inEdgesOf, g.outEdgesOf, g.nodes.set(node->guid, nullptr)), // 在节点列表 nodes 中删掉 node
            g.partyCnt,
            g.T,
            g.randomNodeCnt(),
            g.bubbleCnt()
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
        constexpr uint8_t FLAG_VISITED = 1;
        constexpr uint8_t FLAG_HONEST_TARGET = 2;
        constexpr uint8_t FLAG_BUBBLE_TARGET = 4;
        int len = nodes.size();
        std::map<shared_ptr<Node>, int> mapNodeIndex;
        uint8_t* flags = new uint8_t[len];
        std::queue<int> q;
        // first term: the number of bubbles
        int numBubbles = 0;
        for (int i = 0; i < len; i++)
            if (nodes[i]->state == Node::BUBBLE)
                ++numBubbles;
        // second term: the honest parties' nodes that are reachable to the corrupted parties
        for (int i = 0; i < len; i++) {
            auto nd = nodes[i];
            mapNodeIndex[nd] = i;
            flags[i] = 0;
            if (nd->isEliminated()) {
                flags[i] |= FLAG_VISITED;
            } else {
                if (nd->party->is_corrupted()) q.push(i);
            }
        }
        // do search
        while (!q.empty()) {
            int i = q.front();
            q.pop();
            auto nd = nodes[i];
            if (nd->isEliminated() || (flags[i] & FLAG_VISITED)) continue;
            flags[i] |= FLAG_VISITED;
            if (nd->party->is_honest()) flags[i] |= FLAG_HONEST_TARGET;
            for (auto e : inEdgesOf[nd->guid]) {
                if (e->isEliminated()) continue;
                for (auto v : e->getInputs()) q.push(mapNodeIndex[v]);
            }
        }
        // sum up result
        int numReachableNodes = 0;
        for (int i = 0; i < len; i++) {
            if (flags[i] & FLAG_BUBBLE_TARGET) {
                numReachableNodes++;
            }
        }
        delete[] flags;
        return Potential{numBubbles, numReachableNodes, nodeSize()};
    }

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
}
}  // end of namespace mpc
