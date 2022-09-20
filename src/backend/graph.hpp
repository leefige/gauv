#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <set>

#include "../mpcgraph/builtin.hpp"
#include "common.hpp"
#include "node.hpp"
#include "operation.hpp"

namespace mpc {

class GraphBase {
   protected:
    NodeVec nodes;
    OpVec edges;

   public:
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

   public:
    enum TransformType {
        TAIL_NODE,
        TAIL_EDGE,
        REVERSE_RECONSTRUCT,
        REVERSE_TRANSIT,
        SIM_POLY,
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
        }
    }

    std::vector<std::pair<Node*, TransformType>> transformTape;

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
                    if(dynamic_cast<const Constant*>(oprand)) continue;
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
    }

    bool hasBubble() const {
        for (auto nd : nodes) {
            if (nd->state == Node::BUBBLE) return true;
        }
        return false;
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
        transformTape.push_back(std::make_pair(node, TAIL_NODE));
        return true;
    }

    bool eliminateTailingEdge(Node* node) {
        if (node->getValidOutDegrees()) return false;
        if (node->getValidInDegrees() <= 1) return false;
        if (node->party->is_corrupted()) return false;
        Operation* kept_edge = nullptr;
        for (auto edge : node->getInputs()) {
            if (edge->isGenerated()) {
                // keep one of the generated edges
                kept_edge = edge;
                break;
            }
        }
        if (kept_edge == nullptr) return false;

        for (auto edge : node->getInputs()) {
            if (edge == kept_edge || edge->isEliminated()) continue;
            edge->markEliminated();
            for (auto nd : edge->getInputs()) {
                nd->markPotential();
            }
        }

        assert(node->state == Node::BUBBLE);
        node->state = Node::VISITED;

        transformTape.push_back(std::make_pair(node, TAIL_EDGE));
        return true;
    }

    bool simulatePolynomial(Node* node) {
        NodeVec srcs, dests;
        srcs.push_back(node);
        OpVec old_edges;
        // FIXME: all edges are from the same polynomial is assumed
        for (auto e : node->getOuputs()) {
            if (!e->isEliminated() && !e->isGenerated() &&
                e->getType() == Operator::EVAL) {
                old_edges.push_back(e);
                srcs.push_back(e->getOutput());
            }
        }
        if (srcs.size() > T + 1) return false;
        if (old_edges.empty()) return false;
        for (auto d : srcs) {
            if (d->isRandom()) return false;
        }

        for (auto e : old_edges) e->markEliminated();
        for (auto v : old_edges.front()->getInputs())
            if (v->isRandom()) {
                if (srcs.size() < T + 1)
                    srcs.push_back(v);
                else
                    dests.push_back(v);
            }
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

        transformTape.push_back(std::make_pair(node, SIM_POLY));
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
        if (corruptedNodes.size() <= T) return false;

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

        transformTape.push_back(std::make_pair(node, REVERSE_RECONSTRUCT));
        return true;
    }

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

        // reverse connections
        edge->markEliminated();
        Operation* new_edge =
            new Operation(Operator::TRANSFER, {node}, src_node);
        new_edge->markGenerated();
        edges.push_back(new_edge);
        src_node->addInputOp(new_edge);
        node->addOutputOp(new_edge);

        // update bubbles
        if (!src_node->party->is_corrupted()) {
            if (1 < src_node->getValidInDegrees())
                src_node->state = Node::BUBBLE;
            else
                src_node->markPotential();
        }

        transformTape.push_back(std::make_pair(node, REVERSE_TRANSIT));
        return true;
    }

    bool tryProving() {
        while (hasBubble()) {
            bool hasChange = false;
            for (auto node : nodes) {
                if (node->state == Node::POTENTIAL ||
                    node->state == Node::BUBBLE) {
                    if (eliminateTailingNode(node) ||
                        eliminateTailingEdge(node) ||
                        simulatePolynomial(node) || reverseReconstruct(node) ||
                        reverseTransit(node)) {
                        hasChange = true;
                        break;
                    }
                }
            }
            if (!hasChange) break;
        }
        return !hasBubble();
    }
};

class SubGraph : public GraphBase {
   public:
    SubGraph() {}
    SubGraph(NodeVec nodes) : GraphBase(nodes) {}
    SubGraph(const SubGraph& rhs) : SubGraph(rhs.nodes) {}
};

}  // end of namespace mpc
