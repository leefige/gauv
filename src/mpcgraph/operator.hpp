#pragma once

#include <ostream>

namespace mpc {

class Expression;

enum class Operator : unsigned int {
    NONE = 0,

    INPUT = 100,
    TRANSFER = 101,

    ADD = 200,
    SUB = 201,
    MUL = 202,
    DIV = 203,
    SCALARMUL = 204,

    EVAL = 300,
    RECONSTRUCT = 301,
    POLILIZE = 302,
};

inline std::ostream& operator<<(std::ostream& o, const Operator& op)
{
    switch (op) {
    case Operator::NONE:
        o << "NONE";
        break;
    case Operator::INPUT:
        o << "INPUT";
        break;
    case Operator::TRANSFER:
        o << "TRANSFER";
        break;
    case Operator::ADD:
        o << "ADD";
        break;
    case Operator::SUB:
        o << "SUB";
        break;
    case Operator::MUL:
        o << "MUL";
        break;
    case Operator::DIV:
        o << "DIV";
        break;
    case Operator::SCALARMUL:
        o << "SCALARMUL";
        break;
    case Operator::EVAL:
        o << "EVAL";
        break;
    case Operator::RECONSTRUCT:
        o << "RECONSTRUCT";
        break;
    case Operator::POLILIZE:
        o << "POLILIZE";
        break;

    default:
        o << "<UNKNOWN_OP>";
        break;
    }
    return o;
}

} /* namespace mpc */
