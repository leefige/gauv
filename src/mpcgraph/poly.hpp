#pragma once

#include "context.hpp"
#include "partydecl.hpp"
#include "expression.hpp"
#include "share.hpp"

namespace mpc
{

class Poly : public Expression {
    const size_t _degree;
    const Placeholder& _C;

    Poly(const Poly&) = delete;
    Poly(Poly&&) = delete;
    Poly& operator=(const Poly&) = delete;
    Poly& operator=(Poly&&) = delete;

    explicit Poly(Context& context, const std::string& name,
            Placeholder& C, size_t degree)
        : Expression(context, name, Equation(Operator::POLILIZE, {&C})),
            _degree(degree), _C(C)
    {
        (void) context.register_poly(name, *this);
    }

public:
    static Poly gen_poly(Context& context,
            Placeholder& C, size_t degree)
    {
        size_t num = context.n_poly();
        std::stringstream ss;
        ss << "poly_" << num;
        return Poly(context, ss.str(), C, degree);
    }

    size_t degree() const { return _degree; }

    const Placeholder& const_term() const { return _C; }

    virtual std::string to_string() const override
    {
        std::stringstream ss;
        ss << "<poly{" << _degree << "}[" << _C.name() << "] " << name() << ">";
        return ss.str();
    }

};

} /* namespace mpc */
