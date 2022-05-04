#pragma once

#include <string>

#include "../mpcgraph/builtin.hpp"
#include "common.hpp"

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
        POTENTIAL,
        BUBBLE,
        ELIMINATED,
    } state;

    static std::string to_string(NodeType node_type) {
        switch (node_type) {
            case INPUT:
                return "IN";
            case OUTPUT:
                return "OUT";
            case RANDOM:
                return "RAND";
            default:
                return "-";
        }
    }
    static std::string to_string(NodeState node_state) {
        switch (node_state) {
            case BUBBLE:
                return "BUBBLE";
            default:
                return "-";
        }
    }

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

    const std::string& getName() const { return name; }

    void setInputOps(OpVec inputOps) { isOutputOf = inputOps; }
    void setOutputOps(OpVec outputOps) { isInputOf = outputOps; }

    void addInputOp(Operation* inputOp) { isOutputOf.emplace_back(inputOp); }
    void addOutputOp(Operation* outputOp) { isInputOf.emplace_back(outputOp); }

    OpVec& getInputs() { return isOutputOf; }
    const OpVec& getInputs() const { return isOutputOf; }
    OpVec& getOutputs() { return isInputOf; }
    const OpVec& getOuputs() const { return isInputOf; }
    Operation* firstValidOutput() const;

    int getInDegrees() { return isOutputOf.size(); }
    int getOutDegrees() { return isInputOf.size(); }

    int getValidInDegrees();
    int getValidOutDegrees();

    bool isInput() { return type == INPUT; }
    bool isOutput() { return type == OUTPUT; }
    bool isRandom() { return type == RANDOM; }
    bool isConstant() { return type == CONSTANT; }

    // Don't use! Mark state instead of removing.
    int removeOutputOp(Operation* outputOp) {
        for (auto it = isInputOf.begin(); it != isInputOf.end(); it++) {
            if (*it == outputOp) {
                isInputOf.erase(it);
                return isInputOf.size();
            }
        }
        return -1;
    }
    int removeInputOp(Operation* inputOp) {
        for (auto it = isOutputOf.begin(); it != isOutputOf.end(); it++) {
            if (*it == inputOp) {
                isOutputOf.erase(it);
                return isOutputOf.size();
            }
        }
        return -1;
    }

    bool markPotential() {
        if (state != BUBBLE && state != ELIMINATED &&
            getValidOutDegrees() == 0) {
            state = POTENTIAL;
            return true;
        }
        return false;
    }

    bool markEliminated() {
        state = ELIMINATED;
        return true;
    }
    bool isEliminated() const { return state == ELIMINATED; }

    /* bool checkValid() {
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
    } */

    std::string to_string() const {
        std::stringstream ss;
        ss << "<node[" << party->name() << ", " << to_string(type) << ", "
           << to_string(state) << "] " << name << ">";
        return ss.str();
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
