#pragma once

#include <assert.h>

#include <immer/array.hpp>

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <set>

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
private:
    int guid = 0;

public:
    // Since we need to transform a graph, we use a persistent adjacent list to represent a graph.
    // Also for efficiency, we do not directly use Node but only its id as index.
    const immer::array<OpVec> inEdgesOf, outEdgesOf;
    const NodeVec nodes;

    GraphBase() {}
    GraphBase(const GraphBase& g):
        inEdgesOf(g.inEdgesOf), outEdgesOf(g.outEdgesOf), nodes(g.nodes) {}
    GraphBase(const immer::array<OpVec> inEdgesOf, const immer::array<OpVec> outEdgesOf, const NodeVec nodes):
        inEdgesOf(inEdgesOf), outEdgesOf(outEdgesOf), nodes(nodes) {}
    ~GraphBase() {}

    inline bool isRandomNode(Node* node) const {
        assert(node->guid < nodes.size());
        return inEdgesOf[node->guid].size() == 0;
    }
    inline bool isRandomNode(int node_id) const {
        assert(node_id < g.nodeCnt);
        return inEdgesOf[node_id].size() == 0;
    }

    inline int inDeg(int node_id) const {
        assert(node_id < nodes.size());
        return inEdgesOf[node_id].size();
    }
    inline int inDeg(Node* node) const {
        assert(node->guid < nodes.size());
        return inEdgesOf[node->guid].size();
    }
    inline int outDeg(int node_id) const {
        assert(node_id < nodes.size());
        return outEdgesOf[node_id].size();
    }
    inline int outDeg(Node* node) const {
        assert(node->guid < nodes.size());
        return outEdgesOf[node->guid].size();
    }

    // TODO: hash
};

/**
 * @brief GraphBase is unaware of corrupted parties, Graph is aware of corrupted parties. Also, there are some information (e.g. randomNodeCnt) to help the proving process are stored in Graph.
 */
class Graph: public GraphBase {
public:
    // meta data
    const int partyCnt;
    const int corruptedPartyCnt;

    const int randomNodeCnt;
    const int bubbleCnt;

    Graph(const GraphBase& g, const int partyCnt, const int corruptedPartyCnt, const int randomNodeCnt, const int bubbleCnt):
        GraphBase(g), partyCnt(partyCnt), corruptedPartyCnt(corruptedPartyCnt), randomNodeCnt(randomNodeCnt), bubbleCnt(bubbleCnt) {}

    Graph eliminateEdge(std::shared_ptr<Operation> edge) const {
        // 在 dst 的入边列表中把 edge 删掉
        auto dst = edge->getOuput();
        auto inEdgesOf = this->inEdgesOf;
        inEdgesOf[dst->guid] = inEdgesOf[dst->guid].erase(inEdgesOf[dst->guid], edge);
        
        // 在 srcs 的出边列表中的 edge 删掉
        auto outEdgesOf = this->outEdgesOf;
        for (auto src: edge->getInputs()) {
            outEdgesOf = outEdgesOf.erase(find(outEdgesOf[src->guid], edge));
        }

        // 计算 random nodes 的数量改变，删边之后是可能导致原来的 dst 变成 random node 的
        int addedRandomNodesCnt = inDeg(dst) == 1;

        return Graph(
            GraphBase(inEdgesOf, outEdgesOf, nodes),
            partyCnt,
            corruptedPartyCnt,
            randomNodeCnt + addedRandomNodesCnt,
            bubbleCnt
        );
    }
    Graph eliminateEdges(const std::vector<std::shared_ptr<Operation>>& edges) const {
        // 这个函数其实反复应用上面那个 eliminateEdge
        GraphBase g(*this);
        for (auto edge: edges)
            g = g.eliminateEdge(edge);
        return g;
    }
    Graph addEdge(std::shared_ptr<Operation> edge) const {
        // 在 dst 的入边列表里加入 edge
        auto dst = edge->getOutput();
        auto inEdgesOf = this->inEdgesOf;
        inEdgesOf[dst->guid] = inEdgesOf[dst->guid].push_back(edge);

        // 在 srcs 的出边列表里加入 edge
        auto outEdgesOf = this->outEdgesOf;
        for (auto src: edge->getInputs()) {
            outEdgesOf = outEdgesOf[src->guid].push_back();
        }

        // 计算 random nodes 的数量的改变，加边之后可能导致原先是 random node 的 dst 变得不再是了
        int eliminatedRandomNodesCnt = inDeg(dst) ==0;

        // 计算 bubble 数量的改变，如果 dst 是 corrupted party 的 input 或者 output 的话，我们要将其视为 bubble
        int addedBubbleCnt = dst->party->is_corrupted() && (dst->isInput() || dst->isOutput());

        return Graph(
            GraphBase(inEdgesOf, outEdgesOf, nodes),
            partyCnt,
            corruptedPartyCnt,
            randomNodeCnt - eliminatedRandomNodesCnt,
            bubbleCnt + addedBubbleCnt
        );
    }
    Graph eliminateNode(Node* node) const {
        Graph g = this->eliminateEdges(inEdgesOf[node->guid]); // 删掉 node 的入边
        g = g.eliminateEdges(outEdgesOf[node->guid]); // 删掉 node 的出边
        return Graph(
            GraphBase(g.inEdgesOf, g.outEdgesOf, g.nodes.set(node->guid, nullptr)), // 在节点列表 nodes 中删掉 node
            g.partyCnt,
            g.corruptedPartyCnt,
            g.randomNodeCnt,
            g.bubbleCnt
        )
    }
}

class GraphProver : public Graph {
    // frontend -> backend map
    std::unordered_map<const Expression*, Node*> frontBackMap;
    // polynomial degrees
    size_t T = 0;
    std::set<const PartyDecl*> srcParties;
    std::set<Node*> searchSet, bubbleSet;

public:
    bool computePotential = false;

    enum TransformType {
        TAIL_NODE,
        TAIL_EDGE,
        REVERSE_RECONSTRUCT,
        REVERSE_TRANSIT,
        SIM_POLY,
        REVERSE_OUTPUT_RECONSTRUCT,
    };
    typedef std::tuple<int, int, int> Potential;
    struct HistEntry {
        Node* node;
        TransformType type;
        Potential potential;
    };

    static bool nodeFromHonest(Node* node) {
        if (!node->party->is_corrupted()) return true;
        Operation* edge = nullptr;
        for (auto e : node->getInputs()) {
            if (!e->isEliminated() && e->getType() == Operator::TRANSIT) {
                edge = e;
                break;
            }
        }
        return (edge != nullptr &&
            !edge->getInputs().front()->party->is_corrupted());
    }

    static auto traceOriginalParty(Node* node) {
        Operation* edge = nullptr;
        for (auto e : node->getInputs()) {
            if (!e->isEliminated() && e->getType() == Operator::TRANSIT) {
                edge = e;
                break;
            }
        }
        if (edge != nullptr)
            return edge->getInputs().front()->party;
        else
            return node->party;
    }

    static auto traceTargetParty(Node* node) {
        Operation* edge = nullptr;
        for (auto e : node->getOuputs()) {
            if (!e->isEliminated() && e->getType() == Operator::TRANSIT) {
                edge = e;
                break;
            }
        }
        if (edge != nullptr)
            return edge->getOutput()->party;
        else
            return node->party;
    }

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
        case REVERSE_OUTPUT_RECONSTRUCT:
            return "REVERSE_OUTPUT_RECONSTRUCT";
        default:
            throw std::runtime_error("Bad TransformType");
        }
    }

    static std::string to_string(Potential p) {
        std::stringstream ss;
        ss << "(" << std::get<0>(p) << "," << std::get<1>(p) << ","
            << std::get<2>(p) << ")";
        return ss.str();
    }

    std::vector<HistEntry> transformTape;

    GraphProver() {}
    GraphProver(NodeVec nodes) : Graph(nodes) {}
    ~GraphProver() {
        std::set<Node*> node_set;
        for (auto v : nodes) node_set.insert(v);
        // remove unreferenced nodes
        for (auto& pair : frontBackMap) {
            if (!node_set.count(pair.second) && pair.second != nullptr)
                delete pair.second;
        }
    }

    size_t getT() { return T; }

    int nodeCnt() const {
        int count = 0;
        for (auto nd : nodes)
            if (!nd->isEliminated()) count++;
        return count;
    }

    int edgeSize() const {
        int count = 0;
        for (auto e : edges)
            if (!e->isEliminated()) count++;
        return count;
    }

    Potential potential() const {
        if (!computePotential) return Potential{0, 0, 0};
        // init search
        constexpr uint8_t FLAG_VISITED = 1;
        constexpr uint8_t FLAG_HONEST_TARGET = 2;
        constexpr uint8_t FLAG_BUBBLE_TARGET = 4;
        int len = nodes.size();
        std::map<Node*, int> mapNodeIndex;
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
            for (auto e : nd->getInputs()) {
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
        // third term: the number of total nodes
        int numNodes = nodeCnt();
        return Potential{numBubbles, numReachableNodes, numNodes};
    }

    Node* importFrontend(const Expression* exp) {
        // hit and return
        auto it = frontBackMap.find(exp);
        if (it != frontBackMap.end()) return it->second;

        Operation* old_operation = nullptr;
        Operation* operation = nullptr;
        Node* old_node = nullptr;
        Node* node = nullptr;
        auto secret = dynamic_cast<const Secret*>(exp);
        auto share = dynamic_cast<const Share*>(exp);
        auto poly = dynamic_cast<const Poly*>(exp);
        auto randomness = dynamic_cast<const Randomness*>(exp);
        auto constant = dynamic_cast<const Constant*>(exp);
        switch (exp->cequation().op()) {
        case Operator::NONE:
            break;
        case Operator::INPUT:
            if (secret != nullptr) {
                // create new secret node
                node = new Node();
                node->name = secret->name();
                node->type = Node::INPUT;
                node->party = secret->party();
                nodes.push_back(node);
            } else if (share != nullptr) {
                // create new share node
                node = new Node();
                node->name = share->name();
                node->type = Node::INPUT;
                node->party = share->party();
                nodes.push_back(node);
            } else if (randomness != nullptr) {
                // create new random node
                node = new Node();
                node->name = exp->name();
                node->type = Node::RANDOM;
                node->party = randomness->party();
                nodes.push_back(node);
            } else if (constant != nullptr) {
                // create new constant node
                // Actually we should not need this if everything goes right. The constant should be skipped.
                node = new Node();
                node->name = exp->name();
                node->type = Node::CONSTANT;
                node->party = constant->party();
                nodes.push_back(node);
            }
            // placeholders not secret are not recorded
            break;
        case Operator::TRANSIT:
        case Operator::ADD:
        case Operator::SUB:
        case Operator::MUL:
        case Operator::DIV:
        case Operator::TYPECAST:
        case Operator::SCALARMUL:
        case Operator::RECONSTRUCT:
            // simple nodes -> node operation
            operation = new Operation(exp->cequation().op());
            for (auto oprand : exp->cequation().coprands()) {
                // skip constant
                if (dynamic_cast<const Constant*>(oprand)) {
                    operation->payload = oprand;
                    continue;
                }
                auto tmp_node = importFrontend(oprand);
                assert(tmp_node != nullptr);
                operation->addInput(tmp_node);
                tmp_node->addOutputOp(operation);
            }
            node = new Node();
            node->name = exp->name();
            node->party = exp->party();
            node->type = Node::NONE;

            node->addInputOp(operation);
            operation->setOutput(node);

            edges.push_back(operation);
            nodes.push_back(node);
            break;
        case Operator::EVAL:
            // poly -> share operation
            assert(exp->cequation().coprands().size() == 1);
            assert(share != nullptr);  // target should be share
            old_node = importFrontend(exp->cequation().coprands().front());
            assert(old_node->getInDegrees() == 1);
            old_operation = old_node->getInputs().front();
            assert(old_operation->getType() == Operator::POLILIZE);

            // copy
            operation = new Operation(*old_operation);
            node = new Node(*old_node);

            // properties
            operation->setType(exp->cequation().op());
            node->name = share->name();
            node->party = share->party();
            node->type = Node::NONE;

            // connections
            operation->setOutput(node);
            node->getInputs().clear();
            node->addInputOp(operation);
            for (auto nd : old_operation->getInputs()) {
                nd->addOutputOp(operation);
            }

            edges.push_back(operation);
            nodes.push_back(node);
            break;
        case Operator::POLILIZE:
            operation = new Operation(Operator::POLILIZE);
            node = new Node();
            assert(poly != nullptr);
            if (T < poly->degree()) T = poly->degree();
            node->name = poly->name();
            node->party = poly->party();
            node->type = Node::OTHERS;
            old_node = importFrontend(&poly->const_term());
            if (old_node != nullptr) {
                operation->addInput(old_node);
                // old_node->addOutputOp(operation);
            }
            for (int i = 0; i < poly->degree(); i++) {
                // make a random node first
                auto random_node = new Node();
                random_node->name =
                    poly->name() + "_coeff_" + std::to_string(i);
                random_node->party = poly->party();
                random_node->type = Node::RANDOM;
                // random_node->addOutputOp(operation);
                operation->addInput(random_node);
                nodes.push_back(random_node);
            }
            operation->setOutput(node);
            node->addInputOp(operation);
            break;
        default:
            break;
        }
        frontBackMap[exp] = node;
        return node;
    }

    friend std::ostream& operator<<(std::ostream& o, const GraphProver& p) {
        NodeVec validNodes;
        OpVec validEdges;
        for (auto nd : p.nodes) {
            if (nd->state != Node::ELIMINATED) validNodes.push_back(nd);
        }
        for (auto edge : p.edges) {
            if (edge->state != Operation::ELIMINATED)
                validEdges.push_back(edge);
        }
        std::sort(validNodes.begin(), validNodes.end(),
            [](Node* l, Node* r) { return l->name < r->name; });
        std::sort(validEdges.begin(), validEdges.end(),
            [](Operation* l, Operation* r) {
                return l->getOutput()->name < r->getOutput()->name;
            });
        if (validNodes.size()) {
            o << validNodes[0]->to_string();
        }
        for (int i = 1; i < validNodes.size(); i++) {
            o << std::endl << validNodes[i]->to_string();
        }
        if (validEdges.size()) {
            if (validNodes.size()) o << std::endl;
            o << validEdges[0]->to_string();
        }
        for (int i = 1; i < validEdges.size(); i++) {
            o << std::endl << validEdges[i]->to_string();
        }
        return o;
    }

    void initOutputNodes() {
        for (auto nd : nodes) {
            if (nd->getOutDegrees() == 0 && nd->type == Node::NONE)
                nd->type = Node::OUTPUT;
        }
    }

    void initSearchState() {
        for (auto nd : nodes) {
            if (nd->getInDegrees() == 0 && !nd->party->is_corrupted() &&
                nd->type != Node::RANDOM) {
                nd->state = Node::BUBBLE;
                bubbleSet.insert(nd);
                searchSet.insert(nd);
            } else if (nd->getOutDegrees() == 0) {
                nd->state = Node::POTENTIAL;
                searchSet.insert(nd);
            } else {
                nd->state = Node::UNVISITED;
            }
        }
        for (auto edge : edges) {
            edge->state = Operation::UNVISITED;
        }
        transformTape.clear();
        srcParties.clear();
    }

    bool hasBubble() const {
        for (auto nd : bubbleSet) {
            if (!nd->isEliminated()) return true;
        }
        return false;
    }

    std::string getHashString() {
        std::string hashstring_graph;
        // edge
        hashstring_graph += "EDGE";
        std::vector<std::string> string_edges;
        for (auto edge : edges) {
            if (edge != nullptr && !edge->isEliminated()) {
                std::string hashstring_edge;
                std::vector<std::string> string_inputs, string_outputs;
                for (auto input : edge->getInputs()) {
                    if (input != nullptr && !input->isEliminated()) {
                        string_inputs.push_back(input->name);
                    }
                }
                if (edge->getOutput() != nullptr &&
                    !edge->getOutput()->isEliminated()) {
                    string_outputs.push_back(edge->getOutput()->name);
                }
                std::sort(string_inputs.begin(), string_inputs.end());
                std::sort(string_outputs.begin(), string_outputs.end());
                std::string hashstring_input = "_INPUT_",
                    hashstring_output = "_OUTPUT_";
                for (auto string_input : string_inputs) {
                    hashstring_input += string_input;
                }
                for (auto string_output : string_outputs) {
                    hashstring_output += string_output;
                }
                hashstring_edge =
                    hashstring_input + hashstring_output +
                    std::to_string(static_cast<unsigned int>(edge->getType()));
                string_edges.push_back(hashstring_edge);
            }
        }
        std::sort(string_edges.begin(), string_edges.end());
        for (auto string_edge : string_edges) {
            hashstring_graph += string_edge;
        }
        // node
        hashstring_graph += "_NODE_";
        for (auto node : nodes) {
            if (node != nullptr && !node->isEliminated()) {
                hashstring_graph += node->name;
            }
        }
        return hashstring_graph;
    }

    bool eliminateTailingNode(Node* node) {
        if (node->getValidOutDegrees()) return false;
        if (node->party->is_corrupted()) return false;
        node->markEliminated();
        for (auto edge : node->getInputs()) {
            edge->markEliminated();
            for (auto nd : edge->getInputs()) {
                nd->markPotential();
                searchSet.insert(nd);
            }
        }
        transformTape.push_back(HistEntry{node, TAIL_NODE, potential()});
        return true;
    }

    bool simulatePolynomial(Node* node) {
        if (node->party->is_corrupted()) return false;
        NodeVec srcs, dests;
        if (!node->isRandom())
            srcs.push_back(node);
        OpVec old_edges;
        // FIXME: all edges are from the same polynomial is assumed
        for (auto e : node->getOuputs()) {
            if (!e->isEliminated() && !e->isGenerated() &&
                e->getType() == Operator::EVAL) {
                old_edges.push_back(e);
                auto n = e->getOutput();
                auto party = traceTargetParty(n);
                if (srcParties.count(party)) {
                    srcs.push_back(n);
                } else if (node->isRandom()
                                // && party->id() <= T
                ) {
                    // 如果 secret 随机的话，我们就需要以 T + 1 个 share 作为 srcs
                    srcs.push_back(n);

                    cout << "Here we simulate a random sharing." << endl;
                }
                else {
                    dests.push_back(n);
                }
            }
        }
        // if (node->name == "r_2_bin" || node->name == "r_1_bin") {
        if (node->name == "share_177") {
            cout << "when trying to simulatePolynomial of " << node->name << ": ";
            if (old_edges.empty()) {
                cout << "there are no old_edges" << endl;
            }
            if (srcs.size() != T + 1) {
                cout << "the size of srcs is " << srcs.size() << ", they are";
                for (Node* src: srcs)
                    cout << " " << src->name << ",";
                cout << endl;
            }
            for (auto d : srcs) {
                if (d->isRandom()) {
                    cout << d->name << " is random" << endl;
                }
                if (d != node && d->getValidInDegrees() != 1) {
                    cout << d->name << " has " << d->getValidInDegrees() << " valid in-degrees" << endl;
                }
            }    
        }
        if (old_edges.empty()) return false;
        if (srcs.size() != T + 1) return false;
        for (auto d : srcs) {
            if (d->isRandom()) return false;
            if (d != node && d->getValidInDegrees() != 1) return false;
        }

        for (auto e : old_edges) e->markEliminated();
        for (auto v : old_edges.front()->getInputs())
            if (v != node) dests.push_back(v);
        for (auto v : srcs)
            if (v != node) v->type = Node::RANDOM;
        for (auto d : dests) {
            if (d->isRandom()) d->type = Node::NONE;
            auto edge = new Operation(Operator::EVAL, srcs, d);
            edge->markGenerated();
            for (auto v : srcs) v->addOutputOp(edge);
            d->addInputOp(edge);
            edges.push_back(edge);

            if (!d->party->is_corrupted() && 1 < d->getValidInDegrees()) {
                d->state = Node::BUBBLE;
                bubbleSet.insert(d);
            } else {
                d->markPotential();
            }
            searchSet.insert(d);
        }

        transformTape.push_back(HistEntry{node, SIM_POLY, potential()});
        return true;
    }

    bool reverseReconstruct(Node* node) {
        if (node->getValidInDegrees() != 1) return false;
        Operation* edge = node->firstValidInput();
        if (edge->getType() != Operator::RECONSTRUCT) return false;
        // avoid loop
        if (edge->isGenerated()) return false;
        // check reconstruct operator
        NodeVec corruptedNodes;
        NodeVec uncorruptedNodes;
        if (node->party->is_corrupted()) {
            corruptedNodes.push_back(node);
        } else {
            uncorruptedNodes.push_back(node);
        }
        for (auto nd : edge->getInputs()) {
            if (nodeFromHonest(nd)) {
                uncorruptedNodes.push_back(nd);
            } else {
                corruptedNodes.push_back(nd);
            };
        }
        if (corruptedNodes.size() <= getT()) return false;

        // reverse connections
        edge->markEliminated();

        for (auto uncor_nd : uncorruptedNodes) {
            Operation* new_edge =
                new Operation(Operator::RECONSTRUCT, corruptedNodes, uncor_nd);
            new_edge->markGenerated();
            edges.push_back(new_edge);

            uncor_nd->addInputOp(new_edge);
            for (auto cor_nd : corruptedNodes) {
                cor_nd->addOutputOp(new_edge);
            }
        }
        // update bubbles
        for (auto uncor_nd : uncorruptedNodes) {
            if (!uncor_nd->party->is_corrupted() &&
                1 < uncor_nd->getValidInDegrees())
                uncor_nd->state = Node::BUBBLE;
            else
                uncor_nd->markPotential();
        }

        transformTape.push_back(
            HistEntry{node, REVERSE_RECONSTRUCT, potential()});
        return true;
    }

private:
    void reverseTransitByEdge(Operation* edge) {
        Node* src_node = edge->getInputs().front();
        Node* node = edge->getOutput();

        // reverse connections
        edge->markEliminated();
        Operation* new_edge =
            new Operation(Operator::TRANSIT, {node}, src_node);
        new_edge->markGenerated();
        edges.push_back(new_edge);
        src_node->addInputOp(new_edge);
        node->addOutputOp(new_edge);
    }

    void reverseAdditionByEdge(Operation* edge) {
        NodeVec src_nodes(edge->getInputs());
        assert(src_nodes.size() == 2);
        Node* a = src_nodes[0], *b = src_nodes[1];
        Node* c = edge->getOutput();
        if ((a->getInDegrees() == 0) != (c->getInDegrees() == 1)) {
            assert((b->getInDegrees() == 0) == (c->getInDegrees() == 1));
            swap(a, b);
        }

        // 由 c = a + b 改成 b = c - a

        edge->markEliminated();
        Operation* new_edge =
            new Operation(Operator::SUB, {c, a}, b);
        new_edge->markGenerated();
        edges.push_back(new_edge);
        b->addInputOp(new_edge);
        a->addOutputOp(new_edge);
        c->addOutputOp(new_edge);
    }

public:
    bool reverseTransit(Node* node) {
        if (node->getValidInDegrees() <= 1) return false;
        Operation* edge = nullptr;
        for (auto e : node->getInputs()) {
            if (!e->isEliminated() && !e->isGenerated() &&
                e->getType() == Operator::TRANSIT) {
                edge = e;
                break;
            }
        }
        if (edge == nullptr) return false;
        Node* src_node = edge->getInputs().front();

        reverseTransitByEdge(edge);

        // update bubbles
        if (!src_node->party->is_corrupted()) {
            if (1 < src_node->getValidInDegrees())
                src_node->state = Node::BUBBLE;
            else
                src_node->markPotential();
        }

        transformTape.push_back(HistEntry{node, REVERSE_TRANSIT, potential()});
        return true;
    }

    bool reverseAddition(Node* node) {
        if (node->getValidInDegrees() != 2) return false;
        Operation* edge = nullptr;
        for (auto e : node->getInputs()) {
            if (!e->isEliminated() &&
                e->getType() == Operator::ADD &&
                (node->getInDegrees() == 1) == ((e->getInputs()[0]->getInDegrees() == 0) + (e->getInputs()[1]->getInDegrees() == 0)) // 零入度的节点数量不变
            ) {
                edge = e;
                break;
            }
        }
        if (edge == nullptr) return false;
        NodeVec src_nodes(edge->getInputs());

        reverseAdditionByEdge(edge);

        // update bubbles
        for (auto src_node : src_nodes)
            if (!src_node->party->is_corrupted()) {
                if (src_node->getValidInDegrees() > 0 && src_node->isInput())
                    src_node->state = Node::BUBBLE;
                else
                    src_node->markPotential();
            }

        transformTape.push_back(HistEntry{node, REVERSE_TRANSIT, potential()});
        return true;
    }

    bool reverseOutputReconstruct(Node* node) {
        if (node->getValidInDegrees() != 1) return false;
        Operation* edge = node->firstValidInput();
        if (edge->getType() != Operator::RECONSTRUCT) return false;
        // avoid loop
        if (edge->isGenerated()) return false;
        // refuse to reverse honest output (which should be eliminated)
        if (node->party->is_honest()) return false;
        NodeVec srcNodes;
        NodeVec dstNodes;
        // output is always src node
        srcNodes.push_back(node);
        // check reconstruct operator
        for (auto nd : edge->getInputs()) {
            auto party = traceOriginalParty(nd);
            if (srcParties.count(party)) {
                srcNodes.push_back(nd);
            } else if (party->is_corrupted()) {
                srcParties.insert(party);
                srcNodes.push_back(nd);
            }
        }
        for (auto nd : edge->getInputs()) {
            auto party = traceOriginalParty(nd);
            if (party->is_honest()) {
                if (srcNodes.size() <= T) {
                    // we borrow some honest sources to get T+1 sources
                    srcParties.insert(party);
                    srcNodes.push_back(nd);
                } else {
                    dstNodes.push_back(nd);
                }
            }
        }
        if (srcNodes.size() <= T || dstNodes.empty()) return false;

        // reverse connections
        edge->markEliminated();

        for (auto dst : dstNodes) {
            Operation* newEdge =
                new Operation(Operator::RECONSTRUCT, srcNodes, dst);
            newEdge->markGenerated();
            edges.push_back(newEdge);

            dst->addInputOp(newEdge);
            dst->markPotential();
            searchSet.insert(dst);
            for (auto src : srcNodes) src->addOutputOp(newEdge);

            // reverse transit
            for (auto edge : dst->getInputs())
                if (!edge->isEliminated() &&
                    edge->getType() == Operator::TRANSIT) {
                    reverseTransitByEdge(edge);
                    auto src = edge->getInputs().front();
                    src->markPotential();
                    searchSet.insert(src);
                    break;
                }
        }

        transformTape.push_back(
            HistEntry{node, REVERSE_OUTPUT_RECONSTRUCT, potential()});
        return true;
    }

    GraphProver* tryProving(std::vector<std::string>& histories) {
        if (!hasBubble()) {
            return this;
        }

        std::string hashString = getHashString();
        for (auto history : histories) {
            if (history == hashString) {
                return nullptr;
            }
        }

        histories.push_back(hashString);
        GraphProver* simulator = nullptr;

        for (int i = 0; i < nodes.size(); i++) {
            if (nodes[i]->state == Node::POTENTIAL ||
                nodes[i]->state == Node::BUBBLE) {
                GraphProver* g_1 = new GraphProver(*this);
                if (g_1->eliminateTailingNode(g_1->nodes[i])) {
                    std::cout << "TAIL_NODE" << std::endl;
                    if ((simulator = g_1->tryProving(histories)) != nullptr) {
                        return simulator;
                    }
                }
            }
        }

        for (int i = 0; i < nodes.size(); i++) {
            if (nodes[i]->state != Node::ELIMINATED) {
                GraphProver* g_2 = new GraphProver(*this);
                if (g_2->reverseOutputReconstruct(g_2->nodes[i])) {
                    std::cout << "REVERSE_OUTPUT_RECONSTRUCT" << std::endl;
                    if ((simulator = g_2->tryProving(histories)) != nullptr) {
                        return simulator;
                    }
                }
            }
        }

        for (int i = 0; i < nodes.size(); i++) {
            if (nodes[i]->state != Node::ELIMINATED) {
                GraphProver* g_3 = new GraphProver(*this);
                if (g_3->simulatePolynomial(g_3->nodes[i])) {
                    std::cout << "SIM_POLY" << std::endl;
                    if ((simulator = g_3->tryProving(histories)) != nullptr) {
                        return simulator;
                    }
                }
                GraphProver* g_4 = new GraphProver(*this);
                if (g_4->reverseReconstruct(g_4->nodes[i])) {
                    std::cout << "REVERSE_RECONSTRUCT" << std::endl;
                    if ((simulator = g_4->tryProving(histories)) != nullptr) {
                        return simulator;
                    }
                }
                GraphProver* g_5 = new GraphProver(*this);
                if (g_5->reverseAddition(g_5->nodes[i])) {
                    std::cout << "REVERSE_ADDITION" << std::endl;
                    if ((simulator = g_5->tryProving(histories)) != nullptr) {
                        return simulator;
                    }
                }
            }
        }

        std::cout << "GO_BACK" << std::endl;
        return nullptr;
    }

    bool tryProvingByGreedyStrategy() {
        // 每次选一个让秩函数下降最多的 transformation 来做。
        computePotential = true;
        return true;
    }

    bool tryProvingByPotential() {
        computePotential = true;
        while (hasBubble()) {
            bool transformed = false;
            // first try to eliminate tailing node
            for (auto node : nodes)
                if (node->state == Node::POTENTIAL ||
                    node->state == Node::BUBBLE)
                    if (eliminateTailingNode(node)) {
                        transformed = true;
                    }
            if (transformed) continue;
            for (auto node : nodes)
                if (node->state != Node::ELIMINATED) {
                    if (reverseOutputReconstruct(node)) {
                        transformed = true;
                    }
                }
            if (transformed) continue;
            for (auto node : nodes)
                if (node->state != Node::ELIMINATED) {
                    if (simulatePolynomial(node) || reverseReconstruct(node) || reverseAddition(node)) {
                        transformed = true;
                        break;
                    }
                }
            if (!transformed) {
                return false;
            }
        }
        return true;
    }

    bool tryProvingByHint() {
        computePotential = false;
        while (hasBubble()) {
            bool transformed = false;
            while (!searchSet.empty()) {
                auto node = *searchSet.begin();
                searchSet.erase(node);
                if (!node->isEliminated()) {
                    if (eliminateTailingNode(node) ||
                        reverseOutputReconstruct(node) ||
                        simulatePolynomial(node)) {
                        transformed = true;
                        break;
                    }
                }
            }
            if (!transformed) return false;
        }
        return true;
    }
};
}  // end of namespace mpc
