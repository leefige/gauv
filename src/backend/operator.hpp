#pragma once

#include <vector>

#include "common.hpp"

namespace mpc {

class Operator {
   public:
    enum OpType {
        NONE = 0,

        ADD = 100,
        SUB,
        MUL,
        DIV,

        EVAL = 200,
        RECONSTRUCT,
    };

    Operator() : guid(generateGuid()), hash(generateHash()) {}
    Operator(OpType type)
        : guid(generateGuid()), hash(generateHash()), type(type) {}
    Operator(OpType type, NodeVec inputs, NodeVec outputs)
        : guid(generateGuid()),
          hash(generateHash()),
          type(type),
          inputs(inputs),
          outputs(outputs) {}
    Operator(const Operator& rhs)
        : guid(generateGuid()), hash(rhs.hash), type(rhs.type) {}

    void clear();

    void setPrecedessors(OpVec pre) { predecessors = pre; }
    void setSuccessors(OpVec suc) { successors = suc; }

    void addPrecedessor(Operator* pre) { predecessors.emplace_back(pre); }
    void addSuccessor(Operator* suc) { successors.emplace_back(suc); }

    OpVec& getPrecedessors() { return predecessors; }
    const OpVec& getPrecedessors() const { return predecessors; }
    OpVec& getSuccessors() { return successors; }
    const OpVec& getSuccessors() const { return successors; }

    void* setInputs(NodeVec inputs_) { inputs = inputs_; }
    void* setOutputs(NodeVec outputs_) { outputs = outputs_; }

    void addInput(Node* input) { inputs.emplace_back(input); }
    void addOutput(Node* output) { outputs.emplace_back(output); }

    NodeVec& getInputs() { return inputs; }
    const NodeVec& getInputs() const { return inputs; }
    NodeVec& getOutputs() { return outputs; }
    const NodeVec& getOuputs() const { return outputs; }

    const OpType getType() const { return type; }

   private:
    const size_t guid;
    uint64_t hash;
    OpType type;
    NodeVec inputs;
    NodeVec outputs;
    OpVec predecessors;
    OpVec successors;
};

}  // end of namespace mpc