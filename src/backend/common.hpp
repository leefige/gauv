#pragma once

#include <cstdlib>
#include <cassert>
#include <memory>
#include <sstream>
#include <random>
#include <vector>
#include <algorithm>

#include <immer/flex_vector.hpp>

namespace mpc {

class Node;
class Operation;
class GraphProver;

using NodeVec = immer::flex_vector<std::shared_ptr<Node>>;
using OpVec = immer::flex_vector<std::shared_ptr<Operation>>;

template<typename T>
inline int find(immer::flex_vector<T> vec, T item) {
    int index = 0;
    for (; index < vec.size(); ++index)
        if (vec[index] == item)
            return index;
    return vec.size();
}

typedef std::tuple<int, int, int, int> Potential;

std::string to_string(Potential p) {
    std::stringstream ss;
    ss << "(" << std::get<0>(p) << "," << std::get<1>(p) << ","
        << std::get<2>(p) << ")";
    return ss.str();
}

}  // end of namespace mpc
