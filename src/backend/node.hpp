#pragma once

#include "common.hpp"
#include "operator.hpp"

#include <string>

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

    Node() : guid(generateGuid()), hash(generateHash()) {}
    Node(std::string name, std::string party, NodeType type=OTHERS, Operator op=Operator::NONE)
        : guid(generateGuid()), hash(generateHash()),
          name(name), party(party), type(type), isResultOf(op) {}
    Node(const Node &rhs)
        : guid(generateGuid()), hash(rhs.hash),
          name(rhs.name), party(rhs.party), type(rhs.type), isResultOf(rhs.isResultOf) {}

    ~Node() {}

    void clear();

    void setPredecessors(NodeVec pre) { predecessors = pre; }
    void setSuccessors(NodeVec suc) { successors = suc; }

    void addPredecessor(Node *pre) { predecessors.emplace_back(pre); }
    void addSuccessor(Node *suc) { successors.emplace_back(suc); }

    NodeVec &getPredecessors() { return predecessors; }
    const NodeVec &getPredecessors() const { return predecessors; }
    NodeVec &getSuccessors() { return successors; }
    const NodeVec &getSuccessors() const { return successors; }

    int getInDegrees() { return predecessors.size(); }
    int getOutDegrees() { return successors.size(); }

    bool isInput() { return type == INPUT; }
    bool isOutput() { return type == OUTPUT; }
    bool isRandom() { return type == RANDOM; }
    bool isConstant() { return type == CONSTANT; }

    bool checkValid();

private:
    const size_t guid;
    uint64_t hash;

    std::string name;
    std::string party;
    NodeType type;
    Operator isResultOf;

    NodeVec predecessors;
    NodeVec successors;
};

}  // end of namespace mpc
