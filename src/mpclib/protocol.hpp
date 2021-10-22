#pragma once

#include "defs.hpp"
#include "mpc_exceptions.hpp"

#include <functional>

namespace mpc {

template<base_t BASE, sec_t T, typename... ArgTypes>
class protocol {
    using PS_T = parset<BASE>;
    using B_T = bundle<BASE>;
    using FUNC_T = std::function<B_T(const ArgTypes&...)>;

    const PS_T& _parties;
    FUNC_T _lambda;

public:
    explicit protocol(const PS_T& parties, const FUNC_T& func) noexcept :
        _parties(parties), _lambda(func) {}

    B_T operator()(const ArgTypes&... args)
    {
        return _lambda(args...);
    }
};

} /* namespace mpc */
