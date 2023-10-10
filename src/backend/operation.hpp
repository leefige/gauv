#pragma once

#include <omp.h>

#include <vector>
#include <memory>

#include "../mpcgraph/builtin.hpp"
#include "common.hpp"

namespace mpc {

class Operation {
    uint64_t generateHash(Operator type, NodeVec inputs, std::shared_ptr<Node> output) {
        uint64_t res = (uint64_t)type * 2333333;
        uint64_t resp = 0;
#ifdef _OPENMP
#pragma omp parallel num_threads(8) reduction(+:resp)
#endif
        for (auto input: inputs)
            resp += input->guid;
        res += resp;
        res *= 2333333; // 2333333 is just some magic number
        if (output != nullptr)
            res += output->guid;
        return res;
    }
    uint64_t generateHash(Operator type, std::vector<std::shared_ptr<Node>> inputs, std::shared_ptr<Node> output) {
        uint64_t res = (uint64_t)type * 2333333;
        uint64_t resp = 0;
#ifdef _OPENMP
#pragma omp parallel num_threads(8) reduction(+:resp)
#endif
        for (auto input: inputs)
            resp += input->guid;
        res += resp;
        res *= 2333333; // 2333333 is just some magic number
        if (output != nullptr)
            res += output->guid;
        return res;
    }

public:
    // enum OperationState {
    //     UNVISITED = 0,
    //     VISITED,
    //     GENERATED,
    //     ELIMINATED,
    // } state;
    Operation() {}
    Operation(Operator type)
        : type(type) {}
    Operation(Operator type, NodeVec inputs, std::shared_ptr<Node> output)
        : hash(generateHash(type, inputs, output)),
          type(type),
          inputs(inputs),
          output(output) {}
    Operation(Operator type, std::vector<std::shared_ptr<Node>> inputs, std::shared_ptr<Node> output)
        : hash(generateHash(type, inputs, output)),
          type(type),
          inputs(inputs.begin(), inputs.end()),
          output(output) {}
    Operation(const Operation& rhs)
        : // state(rhs.state),
          payload(rhs.payload),
          hash(rhs.hash),
          type(rhs.type),
          inputs(rhs.inputs),
          output(rhs.output) {}

    void setInputs(NodeVec inputs_) {
        inputs = inputs_;
        hash = generateHash(type, inputs, output);
    }
    void setOutput(std::shared_ptr<Node> output_) {
        output = output_;
        hash = generateHash(type, inputs, output);
    }

    void addInput(std::shared_ptr<Node> input) {
        inputs = inputs.push_back(input);
        hash = generateHash(type, inputs, output);
    }

    NodeVec& getInputs() { return inputs; }
    const NodeVec& getInputs() const { return inputs; }
    std::shared_ptr<Node> getOutput() const { return output; }

    Operator getType() const { return type; }
    void setType(Operator type_) {
        type = type_;
        hash = generateHash(type, inputs, output);
    }

    // bool markGenerated() {
    //     state = GENERATED;
    //     return true;
    // }
    // bool isGenerated() const { return state == GENERATED; }

    // bool markEliminated() {
    //     state = ELIMINATED;
    //     return true;
    // }
    // bool isEliminated() const { return state == ELIMINATED; }

    std::string to_string() const;

    void* payload = nullptr;

    uint64_t hash = 0;

   private:
    Operator type;
    NodeVec inputs;
    std::shared_ptr<Node> output;

    // 事实上，除了 operator 之外，严格来讲我们还需要有 argument，比如说数乘具体是乘多少，插值具体是在哪些点上插值，这里简单起见我们可以先忽略 argument。
};

}  // end of namespace mpc
