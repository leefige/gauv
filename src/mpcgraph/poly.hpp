#pragma once

#include "context.hpp"
#include "expression.hpp"

namespace mpc
{

class Poly : public Expression {
    const Secret& _secret;

public:
    explicit Poly(Context& context, const Secret& secret) noexcept
        : Expression(context), _secret(secret)
    {
        (void) context.register_poly(*this);
    }



}

} /* namespace mpc */
