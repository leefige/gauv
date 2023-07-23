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
        GENERATED,
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
        : state(rhs.state),
          payload(rhs.payload),
          guid(generateGuid()),
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

    bool markGenerated() {
        state = GENERATED;
        return true;
    }
    bool isGenerated() const { return state == GENERATED; }

    bool markEliminated() {
        state = ELIMINATED;
        return true;
    }
    bool isEliminated() const { return state == ELIMINATED; }

    std::string to_string() const;

    void* payload = nullptr;

   private:
    const size_t guid;
    uint64_t hash;
    Operator type;
    NodeVec inputs;
    Node* output;

    // 事实上，除了 operator 之外，严格来讲我们还需要有 argument，比如说数乘具体是乘多少，插值具体是在哪些点上插值，这里简单起见我们可以先忽略 argument。
};

}  // end of namespace mpc