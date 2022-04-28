#include "operator.hpp"

namespace mpc {

void Operator::clear() {
    inputs.clear();
    outputs.clear();
    predecessors.clear();
    successors.clear();
}

}  // end of namespace mpc