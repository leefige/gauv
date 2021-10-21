#pragma once

#include "field.hpp"
#include "party.hpp"
#include "share.hpp"

#include <array>

#include <cstdint>

namespace mpc {

using sec_t = uint32_t;

template<base_t BASE, sec_t DEG>
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
    poly(const P& party, const F& secret, const std::array<F, DEG>& coeffs) noexcept :
        _party(party), _secret(secret), _coeffs(coeffs) {}

    share<BASE> eval(const P& party) const
    {
        F res = _secret;
        F factor(1);
        for (int i = 0; i < DEG; i++) {
            factor *= party.alpha();
            res += factor * _coeffs[i];
        }
        return share<BASE>(party, res);
    }

    std::string to_string() const
    {
        std::stringstream ss;
        ss << "<poly{" << _party << "} " << _secret;
        for (int i = 0; i < DEG; i++) {
            ss << "+" << _coeffs[i] << "*x^" << i + 1;
        }
        ss << ">";
        return ss.str();
    }

    friend std::ostream& operator<<(std::ostream& o, const poly<BASE, DEG>& p)
    {
        return o << p.to_string();
    }
};

} /* namespace mpc */
