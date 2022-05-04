#pragma once

#include <algorithm>
#include <iostream>
#include <memory>

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
        // TODO: deallocate unused node when import frontend
        // FIXME: does all graphs have strong reference to nodes?
        for (auto node : nodes)
            if (node != nullptr) delete node;
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
        if (p.nodes.size()) {
            o << p.nodes[0]->to_string();
        }
        for (int i = 1; i < p.nodes.size(); i++) {
            o << std::endl << p.nodes[i]->to_string();
        }
        if (p.edges.size()) {
            if (p.nodes.size()) o << std::endl;
            o << p.edges[0]->to_string();
        }
        for (int i = 1; i < p.edges.size(); i++) {
            o << std::endl << p.edges[i]->to_string();
        }
        return o;
    }
};

class SubGraph : public GraphBase {
   public:
    SubGraph() {}
    SubGraph(NodeVec nodes) : GraphBase(nodes) {}
    SubGraph(const SubGraph& rhs) : SubGraph(rhs.nodes) {}
};

}  // end of namespace mpc
