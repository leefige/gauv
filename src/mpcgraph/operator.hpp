#pragma once

#include <ostream>

namespace mpc {

class Expression;

enum class Operator : unsigned int {
    NONE = 0,

    INPUT = 100,
    TRANSIT = 101,

    ADD = 200,
    SUB = 201,
    MUL = 202,
    DIV = 203,
    SCALARMUL = 204,

    EVAL = 300,
    RECONSTRUCT = 301, // Xingyu：自然，reconstruct 也是可以换成 EVAL 的，不过这里我们方便起见，还是暂且保留这个 RECONSTRUCT 吧
    POLILIZE = 302, // 2023-07-21 17:50:12 事实上这个 operator 已经不会出现在图里了，因为我们在 bgwfrontend 会直接使用 EVAL

    TYPECAST = 400,
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
    case Operator::TRANSIT:
        o << "TRANSIT";
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
    case Operator::TYPECAST:
        o << "TYPECAST";
        break;
    default:
        o << "<UNKNOWN_OP>";
        break;
    }
    return o;
}

} /* namespace mpc */
