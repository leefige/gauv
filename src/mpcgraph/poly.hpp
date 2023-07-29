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
    const PartyDecl* _party;
    const Expression& _C;

    Poly(const Poly&) = delete;
    Poly(Poly&&) = delete;
    Poly& operator=(const Poly&) = delete;
    Poly& operator=(Poly&&) = delete;

    explicit Poly(Context& context,
            const std::string& name,
            const PartyDecl* party,
            Expression& C,
            size_t degree)
        : Expression(context, name, PolyType::get_poly_type(C.type()), Equation(Operator::POLILIZE, {&C})),
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
        spdlog::trace("~Poly {}", name());
    }

    static Poly& gen_poly(Context& context, const PartyDecl* party,
            Expression& C, size_t degree)
    {
        size_t num = context.n_poly();
        std::stringstream ss;
        ss << "poly_" << num;
        auto ret = new Poly(context, ss.str(), party, C, degree);
        return *ret;
    }

    size_t degree() const { return _degree; }

    const Expression& const_term() const { return _C; }
    const PartyDecl* party() const { return _party; }

    Share& eval(PartyDecl& party)
    {
        size_t num = context().n_share();
        std::stringstream ss;
        ss << "share_" << num;

        auto ret = new Share(
            context(),
            ss.str(),
            dynamic_cast<PolyType *>(this->type())->elem_type,
            Equation(Operator::EVAL, {this}),
            _party
        );
        ret->equation().params.push_back(reinterpret_cast<void*>(&party));
        return *ret;
    }

    virtual std::string to_string() const override
    {
        std::stringstream ss;
        ss << "<poly{" << _degree << "}[" << _party->name() << "][" << _C.name() << "] " << name() << ">";
        return ss.str();
    }

};

} /* namespace mpc */
