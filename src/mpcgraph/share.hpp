#pragma once

#include <spdlog/spdlog.h>

#include "expression.hpp"
#include "excepts.hpp"
#include "context.hpp"
#include "partydecl.hpp"

#include <vector>
#include <memory>

#include <iostream>

namespace mpc {

class Poly;

class Share : public Expression {
    friend Poly;

    Share(const Share&) = delete;
    Share(Share&&) = delete;
    Share& operator=(const Share&) = delete;
    Share& operator=(Share&&) = delete;

    explicit Share(Context& context,
            const std::string& name,
            Type* type,
            PartyDecl* party) noexcept
        : Expression(context, name, type, Equation(Operator::INPUT, {}), party)
    {
        (void) context.register_share(name, this);
    }

    explicit Share(Context& context,
            const std::string& name,
            Type* type,
            const Equation& eqn,
            PartyDecl* party) noexcept
        : Expression(context, name, type, eqn, party)
    {
        (void) context.register_share(name, this);
    }

    friend Share& _binary_op(Share& a, Share& b, Operator op)
    {
        if (a.party() != b.party()) {
            throw party_mismatch(a.party(), b.party());
        }
        

        auto& ctx = Context::get_context();
        size_t num = ctx.n_share();
        std::stringstream ss;
        ss << "share_" << num;

        auto ret = new Share(
            ctx,
            ss.str(),
            a.type(),
            Equation(op, {&a, &b}),
            a.party()
        );
        return *ret;
    }

public:
    virtual ~Share() {}

    static Share& gen_share(Context& context, Type* type, PartyDecl* party)
    {
        size_t num = context.n_share();
        std::stringstream ss;
        ss << "share_" << num;
        auto ret = new Share(context, ss.str(), type, party);
        return *ret;
    }

    virtual std::string to_string() const override
    {
        std::stringstream ss;
        ss << "<share[" << party()->name() << "] "
            << name() << "=" << cequation().op() << "(";

        for (auto cit = cequation().coprands().cbegin();
                cit != cequation().coprands().cend(); cit++) {
            if (cit != cequation().coprands().cbegin()) {
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

    Share& scalarMul(Constant& c)
    {
        auto& ctx = Context::get_context();
        size_t num = ctx.n_share();
        std::stringstream ss;
        ss << "share_" << num;

        auto ret = new Share(
            ctx,
            ss.str(),
            type(),
            Equation(Operator::SCALARMUL, {this, &c}),
            party()
        );
        return *ret; 
    }

    friend Share& operator*(Constant& c, Share& b)
    {
        return b.scalarMul(c);
    }
    friend Share& operator*(Share& a, Constant& c)
    {
        return a.scalarMul(c);
    }

    Share& transfer(PartyDecl* party)
    {
        if (this->party() == party) {
            return *this;
        }

        auto& ctx = Context::get_context();
        size_t num = ctx.n_share();
        std::stringstream ss;
        ss << "share_" << num;

        auto ret = new Share(
            ctx,
            ss.str(),
            type(),
            Equation(Operator::TRANSIT, {this}),
            party
        );
        return *ret;
    }

    static Share& reconstruct(std::vector<Expression*>& shares, PartyDecl* party, std::string name="")
    {
        Type *type = shares[0]->type();
        for (size_t i = 1; i < shares.size(); ++i) {
            if (shares[i]->type() != type) {
                throw type_mismatch(type, shares[i]->type());
            }
        }
        // TODO: to be honest, I think we need more checks here.
        // We need to check if the shares comes from the same polynomial and the shares are more than the degree of the polynomial.
        // Note that this polynomial is not a explicit `Expression`, so it is a little hard to describe in the current framework yet...

        PartyDecl* party_0 = dynamic_cast<const Share*>(shares[0])->party();
        for (auto share : shares) {
            PartyDecl* party_i = dynamic_cast<const Share*>(share)->party();
            if (party_0 != party_i) {
                throw party_mismatch(party_0, party_i);
            }
        }

        Share* ret;
        auto& ctx = Context::get_context();
        if (name == "") {
            size_t num = ctx.n_share();
            std::stringstream ss;
            ss << "share_" << num;

            ret = new Share(
                ctx,
                ss.str(),
                type,
                Equation(Operator::RECONSTRUCT, shares),
                party
            );
        } else {
            ret = new Share(
                ctx,
                name,
                type,
                Equation(Operator::RECONSTRUCT, shares),
                party
            );
        }
        return *ret;
    }
};

} /* namespace mpc */
