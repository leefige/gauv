#include "node.hpp"

namespace mpc {

void Node::clear()  {
    predecessors.clear();
    successors.clear();
}

bool Node::checkValid() {
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
}

}  // end of namespace mpc