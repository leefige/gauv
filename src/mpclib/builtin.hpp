#pragma once

#include "defs.hpp"
#include "random.hpp"

#include <array>

namespace mpc {

#define proto_out(x) return (x)

template<base_t BASE, sec_t DEG>
poly<BASE, DEG> poly_gen(const par<BASE>& party, const field<BASE>& secret)
{
    // TODO: check DEG and party.size()
    std::array<field<BASE>, DEG> coef{field<BASE>(0)};
    for (int i = 0; i < DEG; i++) {
        coef[i] = rand_field<BASE>();
    }

    return poly<BASE, DEG>(party, secret, coef);
}

} /* namespace mpc */
