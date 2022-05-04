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

   public:
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
                assert(exp->cequation().coprands().size() == 1);
                assert(share != nullptr);  // transfer target should be share
                node = importFrontend(exp->cequation().coprands().front());
                // transfer passes on the node
                break;
            case Operator::ADD:
            case Operator::SUB:
            case Operator::MUL:
            case Operator::DIV:
                // simple nodes->node operation
                operation = new Operation(exp->cequation().op());
                for (auto operand : exp->cequation().coprands()) {
                    auto tmp_node = importFrontend(operand);
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
            case Operator::RECONSTRUCT:
                // oply -> share operation
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
        return true;
    }

    bool eliminateTailingEdge(Node* node) {
        // TODO
        return false;
    }

    bool simulatePolynomial(Node* node) {
        if (node->getValidOutDegrees() != 1) return false;
        Operation* edge = node->firstValidOutput();
        if (edge->getType() != Operator::EVAL) return false;
        auto dst = edge->getOutput();
        if (dst->getValidInDegrees() != 1) return false;
        // check eval operator
        int random_count = 0;
        for (auto nd : edge->getInputs()) {
            if (nd->type == Node::RANDOM) random_count++;
        }
        if (random_count < edge->getInputs().size() - 1) return false;

        // create replacement random node
        auto new_dst = new Node(*dst);
        new_dst->name = "sim_" + dst->getName();
        new_dst->type = Node::RANDOM;
        new_dst->state = Node::UNVISITED;
        new_dst->getInputs().clear();
        nodes.push_back(new_dst);

        // change all reference to dst to new_dst
        dst->markEliminated();
        edge->markEliminated();
        for (auto e : new_dst->getOuputs()) {
            if (e->isEliminated()) continue;
            for (auto it = e->getInputs().begin(); it != e->getInputs().end();
                 it++) {
                if (*it == dst) *it = new_dst;
            }
        }

        for (auto nd : edge->getInputs()) {
            if (nd->type == Node::RANDOM) {
                nd->markEliminated();
            } else {
                // FIXME:
                // 这里并未假设node一定是secret，假如是普通的share那么可以做这个消去吗？
                nd->markPotential();
            }
        }

        return true;
    }

    bool tryProving() {
        while (hasBubble()) {
            bool hasChange = false;
            for (auto node : nodes) {
                if (node->state == Node::POTENTIAL ||
                    node->state == Node::BUBBLE) {
                    if (eliminateTailingNode(node)) {
                        hasChange = true;
                        break;
                    }
                    if (simulatePolynomial(node)) {
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
