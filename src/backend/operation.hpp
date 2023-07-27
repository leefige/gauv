#pragma once

#include <vector>
#include <memory>

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
    Operation() : hash(generateHash()) {}
    Operation(Operator type)
        : hash(generateHash()), type(type) {}
    Operation(Operator type, NodeVec inputs, std::shared_ptr<Node> output)
        : hash(generateHash()),
          type(type),
          inputs(inputs),
          output(output) {}
    Operation(Operator type, std::vector<std::shared_ptr<Node>> inputs, std::shared_ptr<Node> output)
        : hash(generateHash()),
          type(type),
          inputs(inputs.begin(), inputs.end()),
          output(output) {}
    Operation(const Operation& rhs)
        : state(rhs.state),
          payload(rhs.payload),
          hash(rhs.hash),
          type(rhs.type),
          inputs(rhs.inputs),
          output(rhs.output) {}

    void setInputs(NodeVec inputs_) { inputs = inputs_; }
    void setOutput(std::shared_ptr<Node> output_) { output = output_; }

    void addInput(std::shared_ptr<Node> input) { inputs = inputs.push_back(input); }

    NodeVec& getInputs() { return inputs; }
    const NodeVec& getInputs() const { return inputs; }
    std::shared_ptr<Node> getOutput() const { return output; }

    Operator getType() const { return type; }
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
    uint64_t hash;
    Operator type;
    NodeVec inputs;
    std::shared_ptr<Node> output;

    // 事实上，除了 operator 之外，严格来讲我们还需要有 argument，比如说数乘具体是乘多少，插值具体是在哪些点上插值，这里简单起见我们可以先忽略 argument。
};

}  // end of namespace mpc