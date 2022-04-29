#include "operation.hpp"

namespace mpc {

void Operation::clear() {
    inputs.clear();
    output = nullptr;
    predecessors.clear();
    successors.clear();
}

}  // end of namespace mpc
