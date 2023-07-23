#pragma once

#include <cstdlib>
#include <cassert>
#include <immer/flex_vector.hpp>
#include <memory>
namespace mpc {

class Node;
class Operation;
class GraphProver;

using NodeVec = immer::flex_vector<std::shared_ptr<Node> >;
using OpVec = immer::flex_vector<std::shared_ptr<Operation> >;

inline size_t generateHash() {
    static uint64_t tag = 0;
    uint64_t hash = std::hash<uint64_t>()(tag++);
    return hash;
}

template<typename T>
inline int find(immer::flex_vector<T> vec, T item) {
    int index = 0;
    for (; index < vec.size(); ++index)
        if (vec[index] == item)
            return index;
    return vec.size();
}

}  // end of namespace mpc
