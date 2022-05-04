#pragma once

#include <vector>

#include "../mpcgraph/builtin.hpp"
#include "common.hpp"

namespace mpc {

class Operation {
   public:
    enum OperationState {
        UNVISITED = 0,
        VISITED,
        ELIMINATED,
    } state;
    Operation() : guid(generateGuid()), hash(generateHash()) {}
    Operation(Operator type)
        : guid(generateGuid()), hash(generateHash()), type(type) {}
    Operation(Operator type, NodeVec inputs, Node* output)
        : guid(generateGuid()),
          hash(generateHash()),
          type(type),
          inputs(inputs),
          output(output) {}
    Operation(const Operation& rhs)
        : guid(generateGuid()),
          hash(rhs.hash),
          type(rhs.type),
          inputs(rhs.inputs),
          output(rhs.output) {}

    void clear() {
        inputs.clear();
        output = nullptr;
    }

    void setInputs(NodeVec inputs_) { inputs = inputs_; }
    void setOutput(Node* output_) { output = output_; }

    void addInput(Node* input) { inputs.emplace_back(input); }

    NodeVec& getInputs() { return inputs; }
    const NodeVec& getInputs() const { return inputs; }
    Node* getOutput() const { return output; }

    const Operator getType() const { return type; }
    void setType(Operator type_) { type = type_; }

   private:
    const size_t guid;
    uint64_t hash;
    Operator type;
    NodeVec inputs;
    Node* output;
};

}  // end of namespace mpc