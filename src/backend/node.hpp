#pragma once

#include "common.h"

namespace mpc {

class Node {
public:
    enum NodeType {
        NONE ,

        OWNED,
        RANDOM,
    };

    Node() : guid(generateGuid()), hash(generateHash()) {}
    Node(NodeType type)
        : guid(generateGuid()), hash(generateHash()), type(type) {}
    Node(const Node &rhs)
        : guid(generateGuid()), hash(rhs.hash), type(rhs.type) {}

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

    bool isOwned() { return type == OWNED; }
    bool isRandom() { return type == RANDOM; }

private:
    const size_t guid;
    uint64_t hash;
    NodeType type;
    NodeVec predecessors;
    NodeVec successors;
};

}  // end of namespace mpc
