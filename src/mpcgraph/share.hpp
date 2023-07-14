#pragma once

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

    const PartyDecl& _party;

    explicit Share(Context& context,
            const std::string& name,
            Type* type,
            const PartyDecl& party) noexcept
        : Expression(context, name, type, Equation(Operator::INPUT, {})), _party(party)
    {
        (void) context.register_share(name, this);
    }

    explicit Share(Context& context,
            const std::string& name,
            Type* type,
            const Equation& eqn,
            const PartyDecl& party) noexcept
        : Expression(context, name, type, eqn), _party(party)
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
            a.type(),
            Equation(op, {&a, &b}),
            a._party
        );
        return *ret;
    }

public:
    virtual ~Share()
    {
        // std::cout << "~Share " << name() << " released" << std::endl;
    }

    static Share& gen_share(Context& context, Type* type, const PartyDecl& party)
    {
        size_t num = context.n_share();
        std::stringstream ss;
        ss << "share_" << num;
        auto ret = new Share(context, ss.str(), type, party);
        return *ret;
    }

    const PartyDecl& party() const { return _party; }

    virtual std::string to_string() const override
    {
        std::stringstream ss;
        ss << "<share[" << _party.name() << "] "
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
            _party
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

    Share& transfer(const PartyDecl& party)
    {
        if (_party == party) {
            return *this;
        }

        auto& ctx = Context::get_context();
        size_t num = ctx.n_share();
        std::stringstream ss;
        ss << "share_" << num;

        auto ret = new Share(
            ctx,
            ss.str(),
            this->type(),
            Equation(Operator::TRANSFER, {this}),
            party
        );
        return *ret;
    }

    static Share& reconstruct(std::vector<Expression*>& shares, const PartyDecl& party)
    {
        Type *type = shares[0]->type();
        for (int i = 1; i < shares.size(); ++i) {
            if (shares[i]->type() != type) {
                throw type_mismatch(type, shares[i]->type());
            }
        }
        // TODO: to be honest, I think we need more checks here.
        // We need to check if the shares comes from the same polynomial and the shares are more than the degree of the polynomial.
        // Note that this polynomial is not a explicit `Expression`, so it is a little hard to describe in the current framework yet...

        const PartyDecl& party_0 = dynamic_cast<const Share*>(shares[0])->_party;
        for (auto share : shares) {
            const PartyDecl& party_i = dynamic_cast<const Share*>(share)->_party;
            if (party_0 != party_i) {
                throw party_mismatch(party_0, party_i);
            }
        }

        auto& ctx = Context::get_context();
        size_t num = ctx.n_share();
        std::stringstream ss;
        ss << "share_" << num;

        auto ret = new Share(
            ctx,
            ss.str(),
            type,
            Equation(Operator::RECONSTRUCT, shares),
            party
        );
        return *ret;
    }
};

} /* namespace mpc */
