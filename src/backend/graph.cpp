#include "graph.hpp"

namespace mpc {

GraphBase::~GraphBase() {
    for (auto node : nodes)
        if (node != nullptr)
            delete node;
}

}  // end of namespace mpc