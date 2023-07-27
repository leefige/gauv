#pragma once

#include "graph.hpp"
#include "immer/flex_vector.hpp"

namespace mpc {
class GraphBaseBuilder {
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<std::vector<std::shared_ptr<Operation>>> inEdgesOf, outEdgesOf;
    std::vector<Expression*> outputs;

    std::shared_ptr<Node> newNode(std::string name, const PartyDecl* party, Node::NodeGenre type) {
        int guid = nodes.size();

        assert(guid == (int)nodes.size());
        assert(guid == (int)inEdgesOf.size());
        assert(guid == (int)outEdgesOf.size());

        auto node = std::make_shared<Node>(guid, name, party, type);
        nodes.push_back(node);
        inEdgesOf.push_back(std::vector<std::shared_ptr<Operation>>());
        outEdgesOf.push_back(std::vector<std::shared_ptr<Operation>>());

        return node;
    }
public:
    GraphBaseBuilder(std::vector<Expression*> outputs): outputs(outputs) {}

    // 从前端建图
    std::shared_ptr<Node> importFrontend(const Expression* exp) {
        // frontend -> backend map
        static std::unordered_map<const Expression*, std::shared_ptr<Node>> frontBackMap;

        // hit and return
        auto it = frontBackMap.find(exp);
        if (it != frontBackMap.end()) return it->second;

        // 这是一个特殊的辅助 map，用来把 poly 节点和它对应的 POLILIZE operation 关联起来
        static std::unordered_map<std::shared_ptr<Node>, std::shared_ptr<Operation>> poly_map;

        std::shared_ptr<Operation> old_operation = nullptr;
        std::shared_ptr<Operation> operation = nullptr;
        std::shared_ptr<Node> old_node = nullptr;
        std::shared_ptr<Node> node = nullptr;
        auto secret = dynamic_cast<const Secret*>(exp);
        auto share = dynamic_cast<const Share*>(exp);
        auto poly = dynamic_cast<const Poly*>(exp);
        auto randomness = dynamic_cast<const Randomness*>(exp);
        switch (exp->cequation().op()) {
        case Operator::NONE:
            break;
        case Operator::INPUT:
            if (secret != nullptr) {
                // create new secret node
                node = newNode(secret->name(), secret->party(), Node::INPUT);
            } else if (share != nullptr) {
                // create new share node
                node = newNode(share->name(), share->party(), Node::INPUT);
            } else if (randomness != nullptr) {
                // create new random node
                node = newNode(randomness->name(), randomness->party(), Node::OTHERS);
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
            operation = std::make_shared<Operation>(exp->cequation().op());
            for (auto oprand : exp->cequation().coprands()) {
                // skip constant
                if (dynamic_cast<const Constant*>(oprand)) {
                    operation->payload = oprand;
                    continue;
                }
                auto tmp_node = importFrontend(oprand);
                assert(tmp_node != nullptr);
                operation->addInput(tmp_node);
                outEdgesOf[tmp_node->guid].push_back(operation);
            }
            node = newNode(exp->name(), exp->party(), Node::OTHERS);

            inEdgesOf[node->guid].push_back(operation);
            operation->setOutput(node);
            break;
        case Operator::EVAL:
            // poly -> share operation
            assert(exp->cequation().coprands().size() == 1);
            assert(share != nullptr);  // target should be share
            old_node = importFrontend(exp->cequation().coprands().front());
            old_operation = poly_map.at(old_node);
            assert(old_operation->getType() == Operator::POLILIZE);

            // copy
            operation = std::make_shared<Operation>(*old_operation);
            operation->setType(exp->cequation().op()); // 其实就是 EVAL

            // new node
            node = newNode(share->name(), share->party(), Node::OTHERS);

            // connections
            operation->setOutput(node);
            inEdgesOf[node->guid].push_back(operation);
            for (auto nd : old_operation->getInputs()) {
                outEdgesOf[nd->guid].push_back(operation);
            }

            break;
        case Operator::POLILIZE:
            // 对于多项式节点我们不调用 newNode，因为它以及 POLILIZE 这条边都是临时的，在上面 EVAL 那个 case 里会被取代掉
            operation = std::make_shared<Operation>(Operator::POLILIZE);
            node = std::make_shared<Node>();
            assert(poly != nullptr);
            node->name = poly->name();
            node->party = poly->party();
            node->type = Node::OTHERS;
            old_node = importFrontend(&poly->const_term());
            if (old_node != nullptr) {
                operation->addInput(old_node);
                // old_node->addOutputOp(operation);
            }
            for (size_t i = 0; i < poly->degree(); i++) {
                // make a random node first
                auto random_node = newNode(poly->name() + "_coeff_" + std::to_string(i), poly->party(), Node::OTHERS);
                // random_node->addOutputOp(operation);
                operation->addInput(random_node);
            }
            operation->setOutput(node);
            poly_map[node] = operation;
            // node->addInputOp(operation);
            break;
        default:
            break;
        }
        frontBackMap[exp] = node;
        return node;
    }

    GraphBase build() {
        // clear
        inEdgesOf.clear();
        outEdgesOf.clear();
        nodes.clear();

        // import outputs
        for (auto output : outputs) {
            auto node = importFrontend(output);
            assert(node != nullptr);
            node->type = Node::OUTPUT;
        }

        // build inEdgesOf and outEdgesOf
        assert(nodes.size() == inEdgesOf.size());
        assert(nodes.size() == outEdgesOf.size());

        std::vector<OpVec> _inEdgesOf, _outEdgesOf;
        for (auto inEdges: inEdgesOf)
            _inEdgesOf.push_back(immer::flex_vector<std::shared_ptr<Operation>>(inEdges.begin(), inEdges.end()));
        for (auto outEdges: outEdgesOf)
            _outEdgesOf.push_back(immer::flex_vector<std::shared_ptr<Operation>>(outEdges.begin(), outEdges.end()));
        immer::flex_vector<OpVec> __inEdgesOf(_inEdgesOf.begin(), _inEdgesOf.end());
        immer::flex_vector<OpVec> __outEdgesOf(_outEdgesOf.begin(), _outEdgesOf.end());
        NodeVec __nodes(nodes.begin(), nodes.end());

        return GraphBase(__inEdgesOf, __outEdgesOf, __nodes);
    }
};
} // namespace mpc
