#pragma once

#include "../mpcgraph/expression.hpp"
#include "node.hpp"
#include "operation.hpp"

namespace mpc {
int Node::getValidInDegrees() {
    int deg = 0;
    for (auto e : isOutputOf)
        if (!e->isEliminated()) deg++;
    return deg;
}
int Node::getValidOutDegrees() {
    int deg = 0;
    for (auto e : isInputOf)
        if (!e->isEliminated()) deg++;
    return deg;
}
Operation* Node::firstValidInput() const {
    for (auto e : isOutputOf)
        if (!e->isEliminated()) return e;
    assert(false);
    return nullptr;
}
Operation* Node::firstValidOutput() const {
    for (auto e : isInputOf)
        if (!e->isEliminated()) return e;
    assert(false);
    return nullptr;
}
std::string Operation::to_string() const {
    std::stringstream ss;
    ss << "<operation[" << type;
    if (type == Operator::SCALARMUL) {
        auto c = (Constant*)(payload);
        ss << " " << c->name();
    }
    ss << "] ";
    if (inputs.size()) {
        ss << inputs[0]->getName();
    }
    for (int i = 1; i < inputs.size(); i++) {
        ss << ", " << inputs[i]->getName();
    }
    ss << " -> ";
    if (output != nullptr) {
        ss << output->getName();
    }
    ss << ">";
    return ss.str();
}
}  // namespace mpc
