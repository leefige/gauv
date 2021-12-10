#pragma once

#include <ostream>

namespace mpc {

class Expression;

enum class Operator {
    NONE,

    INPUT,
    TRANSFER,

    ADD,
    SUB,
    MUL,
    DIV,

    EVAL,
    RECONSTRUCT,
};

std::ostream& operator<<(std::ostream& o, const Operator& op)
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
    case Operator::EVAL:
        o << "EVAL";
        break;
    case Operator::RECONSTRUCT:
        o << "RECONSTRUCT";
        break;

    default:
        o << "<UNKNOWN_OP>";
        break;
    }
    return o;
}

std::stringstream& operator<<(std::stringstream& o, const Operator& op)
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
    case Operator::EVAL:
        o << "EVAL";
        break;
    case Operator::RECONSTRUCT:
        o << "RECONSTRUCT";
        break;

    default:
        o << "<UNKNOWN_OP>";
        break;
    }
    return o;
}

} /* namespace mpc */
