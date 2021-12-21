#pragma once

#pragma once

#include "expression.hpp"
#include "excpts.hpp"
#include "context.hpp"
#include "partydecl.hpp"

#include <vector>
#include <memory>

#include <iostream>

namespace mpc {

class party_mismatch : public std::exception {
    std::string msg;
public:
    party_mismatch(const PartyDecl& want, const PartyDecl& get) noexcept
    {
        std::stringstream ss;
        ss << "Wants " << want << ", gets " << get;
        msg = ss.str();
    }

    virtual const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

class party_duplicated : public std::exception {
    std::string msg;
public:
    party_duplicated(const PartyDecl& party) noexcept
    {
        std::stringstream ss;
        ss << "Wants a party other than " << party;
        msg = ss.str();
    }

    virtual const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

class Poly;

class Share : public Expression {
    friend Poly;

    Share(const Share&) = delete;
    Share(Share&&) = delete;
    Share& operator=(const Share&) = delete;
    Share& operator=(Share&&) = delete;

    const PartyDecl& _party;

    explicit Share(Context& context,
            const std::string& name,
            const Equation& eqn,
            const PartyDecl& party) noexcept
        : Expression(context, name, eqn), _party(party)
    {
        (void) context.register_share(name, this);
    }

    friend Share& _binary_op(Share& a, Share& b, Operator op)
    {
        if (a._party != b._party) {
            throw party_mismatch(a._party, b._party);
        }

        auto& ctx = Context::get_context();
        size_t num = ctx.n_share();
        std::stringstream ss;
        ss << "share_" << num;

        auto ret = new Share(
            ctx,
            ss.str(),
            Equation(op, {&a, &b}),
            a._party
        );
        return *ret;
    }

public:
    virtual ~Share()
    {
        std::cout << "Share " << name() << " released" << std::endl;
    }

    const PartyDecl& party() const { return _party; }

    virtual std::string to_string() const override
    {
        std::stringstream ss;
        ss << "<share[" << _party.name() << "] "
            << name() << "=" << equation().op() << "(";

        for (auto cit = equation().coprands().cbegin();
                cit != equation().coprands().cend(); cit++) {
            if (cit != equation().coprands().cbegin()) {
                ss << ",";
            }
            ss << (*cit)->to_string();
        }

        ss << ")" << ">";
        return ss.str();
    }

    friend Share& operator+(Share& a, Share& b)
    {
        return _binary_op(a, b, Operator::ADD);
    }

    friend Share& operator-(Share& a, Share& b)
    {
        return _binary_op(a, b, Operator::SUB);
    }

    friend Share& operator*(Share& a, Share& b)
    {
        return _binary_op(a, b, Operator::MUL);
    }

    friend Share& operator/(Share& a, Share& b)
    {
        return _binary_op(a, b, Operator::DIV);
    }

    Share& transfer(const PartyDecl& party)
    {
        if (_party == party) {
            throw party_duplicated(party);
        }

        auto& ctx = Context::get_context();
        size_t num = ctx.n_share();
        std::stringstream ss;
        ss << "share_" << num;

        auto ret = new Share(
            ctx,
            ss.str(),
            Equation(Operator::TRANSFER, {this}),
            party
        );
        return *ret;
    }
};

} /* namespace mpc */
