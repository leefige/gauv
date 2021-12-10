#pragma once

#include <initializer_list>
#include <vector>
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

class Equation {
    const Operator _op;
    std::vector<Expression*> _oprands;

    // Equation(const Equation&) = delete;
    // Equation(Equation&&) = delete;
    // Equation& operator=(const Equation&) = delete;
    // Equation& operator=(Equation&&) = delete;

public:
    static const Equation nulleqn;

    // TODO: ensure oprands in the same context
    explicit Equation(const Operator& op,
        const std::initializer_list<Expression*>& oprands) noexcept
        : _op(op), _oprands(oprands) {}

    explicit operator bool() const noexcept { return this != &nulleqn; }

    const Operator& op() const { return _op; }
    std::vector<Expression*>& oprands() { return _oprands; }
};

class NullEquation : private Equation {
    explicit NullEquation() noexcept : Equation(Operator::NONE, {}) {}
public:
    static NullEquation& self() noexcept
    {
        static NullEquation _nulleqn;
        return _nulleqn;
    }
};

const Equation Equation::nulleqn(Operator::NONE, {});

NullEquation& nulleqn = NullEquation::self();

} /* namespace mpc */
