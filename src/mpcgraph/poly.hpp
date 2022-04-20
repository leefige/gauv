#pragma once

#include "context.hpp"
#include "partydecl.hpp"
#include "expression.hpp"
#include "share.hpp"

#include <iostream>

namespace mpc
{

class Poly : public Expression {
    const size_t _degree;
    const PartyDecl& _party;
    const Placeholder& _C;

    Poly(const Poly&) = delete;
    Poly(Poly&&) = delete;
    Poly& operator=(const Poly&) = delete;
    Poly& operator=(Poly&&) = delete;

    explicit Poly(Context& context,
            const std::string& name,
            const PartyDecl& party,
            Placeholder& C,
            size_t degree)
        : Expression(context, name, Equation(Operator::POLILIZE, {&C})),
            _degree(degree), _party(party), _C(C)
    {
        if(Secret* s = dynamic_cast<Secret*>(&C)) {
            // is instance of secret
            if (s->party() != party) {
                throw party_mismatch(party, s->party());
            }
        }
        (void) context.register_poly(name, this);
    }

public:
    ~Poly()
    {
        // std::cout << "~Poly " << name() << " released" << std::endl;
    }

    static Poly& gen_poly(Context& context, PartyDecl& party,
            Placeholder& C, size_t degree)
    {
        size_t num = context.n_poly();
        std::stringstream ss;
        ss << "poly_" << num;
        auto ret = new Poly(context, ss.str(), party, C, degree);
        return *ret;
    }

    size_t degree() const { return _degree; }

    const Placeholder& const_term() const { return _C; }

    Share& eval(PartyDecl& party)
    {
        size_t num = context().n_share();
        std::stringstream ss;
        ss << "share_" << num;

        auto ret = new Share(
            context(),
            ss.str(),
            Equation(Operator::EVAL, {this}),
            _party
        );
        ret->equation().params.push_back(reinterpret_cast<void*>(&party));
        return *ret;
    }

    virtual std::string to_string() const override
    {
        std::stringstream ss;
        ss << "<poly{" << _degree << "}[" << _party.name() << "][" << _C.name() << "] " << name() << ">";
        return ss.str();
    }

};

} /* namespace mpc */