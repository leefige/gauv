#pragma once

#include "field.hpp"
#include "party.hpp"

#include <array>

#include <cstdint>

namespace mpc {

template<base_t BASE, uint32_t DEG>
class poly {
    using F = field<BASE>;
    using P = par<BASE>;

    const P& _party;
    const F _secret;
    const std::array<F, DEG> _coeffs;

public:
    /*
        q_t(x) = secret + a_1 * x^1 + a_2 * x^2 + ... + a_t * x^t.
        coeffs: {a_1, a_2, ..., a_t}
     */
    poly(const P& party, const F& secret, const std::array<F, DEG>& coeffs) :
        _party(party), _secret(secret), _coeffs(coeffs) {}

    F eval(const P& party) const
    {
        F res = _secret;
        F factor(1);
        for (int i = 0; i < DEG; i++) {
            factor *= party.alpha();
            res += factor * _coeffs[i];
        }
        return res;
    }
};

} /* namespace mpc */
