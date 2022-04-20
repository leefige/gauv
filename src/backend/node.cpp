#include "graph.hpp"

namespace mpc {

void Node::clear()  {
    predecessors.clear();
    successors.clear();
}

}  // end of namespace mpc