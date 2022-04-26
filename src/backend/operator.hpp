#pragma once

#include "common.hpp"

namespace mpc {

enum Operator {
    NONE = 0,

    ADD = 100,
    SUB,
    MUL,
    DIV,

    EVAL = 200,
    RECONSTRUCT,
};

}  // end of namespace mpc