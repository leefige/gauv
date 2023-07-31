#pragma once

#include "../mpcgraph/expression.hpp"
#include "node.hpp"
#include "operation.hpp"

namespace mpc {
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
    for (size_t i = 1; i < inputs.size(); i++) {
        ss << ", " << inputs[i]->getName();
    }
    ss << " -> ";
    if (output != nullptr) {
        ss << output->getName();
    }
    ss << ">";
    
    ss << " hash: " << hash;

    return ss.str();
}
}  // namespace mpc
