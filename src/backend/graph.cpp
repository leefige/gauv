#include "graph.hpp"

#include "../mpcgraph/builtin.hpp"

namespace mpc {

GraphBase::~GraphBase() {
    // TODO: deallocate unused node when import frontend
    // FIXME: does all graphs have strong reference to nodes?
    for (auto node : nodes)
        if (node != nullptr) delete node;
}

Node* Graph::importFrontend(const Expression* exp) {
    // hit and return
    auto it = frontBackMap.find(exp);
    if (it != frontBackMap.end()) return it->second;

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
            old_node = importFrontend(exp->cequation().coprands().front());
            // copy and rewrite node name and party
            node = new Node(*old_node);
            node->name = share->name();
            node->party = &share->party();
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
                tmp_node->addInputOp(operation);
            }
            node = new Node();
            assert(share != nullptr);
            node->name = share->name();
            node->party = &share->party();
            node->type = Node::NONE;
            node->addOutputOp(operation);
            operation->setOutput(node);
            nodes.push_back(node);
            break;
        case Operator::EVAL:
        case Operator::RECONSTRUCT:
            assert(exp->cequation().coprands().size() == 1);
            assert(share != nullptr);  // target should be share
            node = importFrontend(exp->cequation().coprands().front());
            node->name = share->name();
            node->party = &share->party();
            node->type = Node::NONE;
            break;
        case Operator::POLILIZE: {
            operation = new Operation(Operator::POLILIZE);
            assert(poly != nullptr);
            auto const_term_node = importFrontend(&poly->const_term());
            // make a random node first
            auto random_node = new Node();
            random_node->name = "coefficient_" + poly->name();
            random_node->party = &poly->party();
            random_node->type = Node::RANDOM;
            random_node->addInputOp(operation);
            operation->addInput(random_node);
            nodes.push_back(random_node);
            if (const_term_node != nullptr) {
                operation->addInput(const_term_node);
                const_term_node->addInputOp(operation);
                nodes.push_back(const_term_node);
            }
            // then the empty output node
            node = new Node();
            operation->setOutput(node);
            node->addOutputOp(operation);
            nodes.push_back(node);
        } break;
        default:
            break;
    }
    frontBackMap[exp] = node;
    return node;
}

}  // end of namespace mpc