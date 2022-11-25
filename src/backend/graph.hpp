#pragma once

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

class GraphBase {
   public:
    NodeVec nodes;
    OpVec edges;

    GraphBase() {}
    GraphBase(NodeVec nodes) : nodes(nodes) {}
    ~GraphBase() {
        for (auto v : nodes)
            if (v != nullptr) delete v;
        for (auto e : edges)
            if (e != nullptr) delete e;
    }

    NodeVec getNodes() { return nodes; };
    const NodeVec getNodes() const { return nodes; };
};

class Graph : public GraphBase {
    // frontend -> backend map
    std::unordered_map<const Expression*, Node*> frontBackMap;
    // polynomial degrees
    size_t T = 0;
    std::set<const PartyDecl*> srcParties;

   public:
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
            if (!e->isEliminated() && e->getType() == Operator::TRANSFER) {
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
            if (!e->isEliminated() && e->getType() == Operator::TRANSFER) {
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
            if (!e->isEliminated() && e->getType() == Operator::TRANSFER) {
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
        }
    }

    static std::string to_string(Potential p) {
        std::stringstream ss;
        ss << "(" << std::get<0>(p) << "," << std::get<1>(p) << ","
           << std::get<2>(p) << ")";
        return ss.str();
    }

    std::vector<HistEntry> transformTape;

    Graph() {}
    Graph(NodeVec nodes) : GraphBase(nodes) {}
    ~Graph() {
        std::set<Node*> node_set;
        for (auto v : nodes) node_set.insert(v);
        // remove unreferenced nodes
        for (auto& pair : frontBackMap) {
            if (!node_set.count(pair.second) && pair.second != nullptr)
                delete pair.second;
        }
    }

    size_t getT() { return T; }

    int nodeSize() const {
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
        // first term
        int numNodes = nodeSize();
        // init search
        constexpr uint8_t FLAG_VISITED = 1;
        constexpr uint8_t FLAG_HONEST_TARGET = 2;
        constexpr uint8_t FLAG_BUBBLE_TARGET = 4;
        int len = nodes.size();
        std::map<Node*, int> mapNodeIndex;
        uint8_t* flags = new uint8_t[len];
        std::queue<int> q;
        // from corrupted
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
        // from bubble
        for (int i = 0; i < len; i++) {
            auto nd = nodes[i];
            mapNodeIndex[nd] = i;
            flags[i] &= ~FLAG_VISITED;
            if (nd->isEliminated()) {
                flags[i] |= FLAG_VISITED;
            } else {
                if (nd->state == Node::BUBBLE) q.push(i);
            }
        }
        // do search
        while (!q.empty()) {
            int i = q.front();
            q.pop();
            auto nd = nodes[i];
            if (nd->isEliminated() || (flags[i] & FLAG_VISITED)) continue;
            flags[i] |= FLAG_VISITED | FLAG_BUBBLE_TARGET;
            for (auto e : nd->getOuputs()) {
                if (e->isEliminated()) continue;
                q.push(mapNodeIndex[e->getOutput()]);
            }
        }
        // sum up result
        int numHonestNodes = 0;
        int numReachableNodes = 0;
        for (int i = 0; i < len; i++) {
            if (flags[i] & FLAG_HONEST_TARGET) {
                numHonestNodes++;
            }
            if (flags[i] & FLAG_BUBBLE_TARGET) {
                numReachableNodes++;
            }
        }
        delete[] flags;
        return Potential{numNodes, numHonestNodes, numReachableNodes};
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
        switch (exp->cequation().op()) {
            case Operator::NONE:
                break;
            case Operator::INPUT:
                if (secret != nullptr) {
                    // create new secret node
                    node = new Node();
                    node->name = secret->name();
                    node->type = Node::INPUT;
                    node->party = &secret->party();
                    nodes.push_back(node);
                }
                // placeholders not secret are not recorded
                break;
            case Operator::TRANSFER:
            case Operator::ADD:
            case Operator::SUB:
            case Operator::MUL:
            case Operator::DIV:
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
                    operation->addInput(tmp_node);
                    tmp_node->addOutputOp(operation);
                }
                node = new Node();
                assert(share != nullptr);
                node->name = share->name();
                node->party = &share->party();
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
                node->party = &share->party();
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
                node->party = &poly->party();
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
                    random_node->party = &poly->party();
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

    friend std::ostream& operator<<(std::ostream& o, const Graph& p) {
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
            } else if (nd->getOutDegrees() == 0) {
                nd->state = Node::POTENTIAL;
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
        for (auto nd : nodes) {
            if (nd->state == Node::BUBBLE) return true;
        }
        return false;
    }

    std::string getHashString() {
        std::string hashstring_graph;
        // edge
        hashstring_graph += "EDGE";
        std::vector<std::string> string_edges;
        for (auto edge : edges) {
            if (!edge->isEliminated()) {
                std::string hashstring_edge;
                std::vector<std::string> string_inputs, string_outputs;
                for (auto input : edge->getInputs()) {
                    if (!input->isEliminated()) {
                        string_inputs.push_back(input->name);
                    }
                }
                if (!edge->getOutput()->isEliminated()) {
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
            if (!node->isEliminated()) {
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
            }
        }
        transformTape.push_back(HistEntry{node, TAIL_NODE, potential()});
        return true;
    }

    bool simulatePolynomial(Node* node) {
        if (node->party->is_corrupted()) return false;
        NodeVec srcs, dests;
        srcs.push_back(node);
        OpVec old_edges;
        // FIXME: all edges are from the same polynomial is assumed
        for (auto e : node->getOuputs()) {
            if (!e->isEliminated() && !e->isGenerated() &&
                e->getType() == Operator::EVAL) {
                old_edges.push_back(e);
                auto node = e->getOutput();
                auto party = traceTargetParty(node);
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

            if (!d->party->is_corrupted() && 1 < d->getValidInDegrees())
                d->state = Node::BUBBLE;
            else
                d->markPotential();
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
            new Operation(Operator::TRANSFER, {node}, src_node);
        new_edge->markGenerated();
        edges.push_back(new_edge);
        src_node->addInputOp(new_edge);
        node->addOutputOp(new_edge);
    }

   public:
    bool reverseTransit(Node* node) {
        if (node->getValidInDegrees() <= 1) return false;
        Operation* edge = nullptr;
        for (auto e : node->getInputs()) {
            if (!e->isEliminated() && !e->isGenerated() &&
                e->getType() == Operator::TRANSFER) {
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
            for (auto src : srcNodes) src->addOutputOp(newEdge);

            // reverse transit
            for (auto edge : dst->getInputs())
                if (!edge->isEliminated() &&
                    edge->getType() == Operator::TRANSFER) {
                    reverseTransitByEdge(edge);
                    break;
                }
        }

        transformTape.push_back(
            HistEntry{node, REVERSE_OUTPUT_RECONSTRUCT, potential()});
        return true;
    }

    bool tryProving(GraphVec& histories) {
        if (!hasBubble()) {
            return true;
        }

        // for (auto history : histories) {
        //     if (history->getHashString() == g->getHashString()) {
        //         return false;
        //     }
        // }

        histories.push_back(this);
        Graph* new_g = new Graph;
        *new_g = *this;

        for (auto node : new_g->nodes) {
            if (node->state == Node::POTENTIAL || node->state == Node::BUBBLE) {
                if (new_g->eliminateTailingNode(node)) {
                    if (new_g->tryProving(histories)) {
                        return true;
                    }
                }
                if (new_g->simulatePolynomial(node)) {
                    if (new_g->tryProving(histories)) {
                        return true;
                    }
                }
                if (new_g->reverseReconstruct(node)) {
                    if (new_g->tryProving(histories)) {
                        return true;
                    }
                }
                if (new_g->reverseTransit(node)) {
                    if (new_g->tryProving(histories)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool tryProvingByPotential() {
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
                    if (simulatePolynomial(node) || reverseReconstruct(node) ||
                        reverseTransit(node)) {
                        transformed = true;
                        break;
                    }
                }
            if (!transformed) return false;
        }
        return true;
    }
};
}  // end of namespace mpc
