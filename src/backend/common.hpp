#pragma once

#include <cstdlib>
#include <cassert>
#include <algorithm>

namespace mpc {

class Node;
class Operation;

using NodeVec = std::vector<Node *>;
using OpVec = std::vector<Operation *>;

inline size_t generateGuid() {
    static size_t guid = 0;
    return guid++;
}

inline size_t generateHash() {
    static uint64_t tag = 0;
    uint64_t hash = std::hash<uint64_t>()(tag++);
    return hash;
}

}  // end of namespace mpc
