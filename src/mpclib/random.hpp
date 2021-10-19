#pragma once

#include "defs.hpp"

#include <random>

namespace mpc {

std::random_device __mpc_random_device;

std::mt19937 __mpc_random_generator(__mpc_random_device());

std::uniform_int_distribution<base_t> __mpc_uniform_base(
    std::numeric_limits<base_t>::min(),
    std::numeric_limits<base_t>::max()
);

template<base_t BASE>
field<BASE> rand_field()
{
    return field<BASE>(__mpc_uniform_base(__mpc_random_generator));
}

} /* namespace mpc */
