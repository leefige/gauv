#pragma once

#include <string>

#include "../mpcgraph/builtin.hpp"
#include "common.hpp"
#include "operation.hpp"

namespace mpc {

class Node {
   public:
    enum NodeType {
        NONE = 0,

        INPUT = 100,
        OUTPUT,
        RANDOM,
        CONSTANT,

        OTHERS = 200,
    };
    enum NodeState {
        UNVISITED = 0,
        VISITED,
        BUBBLE,
        ELIMINATED,
    } state;

    Node() : guid(generateGuid()), hash(generateHash()) {}
    Node(std::string name, PartyDecl* party, OpVec isOutputof, OpVec isInputof,
         NodeType type = OTHERS)
        : guid(generateGuid()),
          hash(generateHash()),
          name(name),
          party(party),
          type(type),
          isOutputOf(isOutputof),
          isInputOf(isInputof) {}
    Node(const Node& rhs)
        : guid(generateGuid()),
          hash(rhs.hash),
          name(rhs.name),
          party(rhs.party),
          type(rhs.type),
          isOutputOf(rhs.isOutputOf),
          isInputOf(rhs.isInputOf) {}

    ~Node() {}

    void clear() {
        isOutputOf.clear();
        isInputOf.clear();
    }

    void setInputOps(OpVec inputOps) { isOutputOf = inputOps; }
    void setOutputOps(OpVec outputOps) { isInputOf = outputOps; }

    void addInputOp(Operation* inputOp) { isOutputOf.emplace_back(inputOp); }
    void addOutputOp(Operation* outputOp) { isInputOf.emplace_back(outputOp); }

    OpVec& getInputs() { return isOutputOf; }
    const OpVec& getInputs() const { return isOutputOf; }
    OpVec& getOutputs() { return isInputOf; }
    const OpVec& getOuputs() const { return isInputOf; }

    int getInDegrees() { return isOutputOf.size(); }
    int getOutDegrees() { return isInputOf.size(); }

    bool isInput() { return type == INPUT; }
    bool isOutput() { return type == OUTPUT; }
    bool isRandom() { return type == RANDOM; }
    bool isConstant() { return type == CONSTANT; }

    bool checkValid() {
        // FIXME: is this intended?
        switch (isOutputOf.front()->getType()) {
            case Operator::NONE:
                return false;
            case Operator::ADD:
                return getInDegrees() == 2;
            case Operator::SUB:
                return getInDegrees() == 2;
            case Operator::MUL:
                return getInDegrees() == 2;
            case Operator::DIV:
                return getInDegrees() == 2;
            case Operator::EVAL:
                return true;
            case Operator::RECONSTRUCT:
                return true;
            default:
                return false;
        }
    }

   private:
    const size_t guid;
    uint64_t hash;

   public:
    std::string name;
    const PartyDecl* party;
    NodeType type;

   private:
    OpVec isOutputOf;
    OpVec isInputOf;
};

}  // end of namespace mpc
