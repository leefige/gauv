#pragma once

#include "field.hpp"

#include <memory>
#include <ostream>
#include <sstream>
#include <initializer_list>

#include <cstdint>

namespace mpc {

using size_t = __ssize_t;

template<base_t BASE>
class par {
    const size_t _idx;
    const field<BASE> _alpha;
public:
    explicit par(size_t idx, base_t alpha) : _idx(idx), _alpha(alpha) {}
    explicit par(size_t idx, field<BASE> alpha) : _idx(idx), _alpha(alpha) {}

    constexpr const field<BASE>& alpha() const { return _alpha; }
    constexpr const size_t& idx() const { return _idx; }

    bool operator==(const par<BASE>& other) const
    {
        return _idx == other._idx && _alpha == other._alpha;
    }

    bool operator!=(const par<BASE>& other) const
    {
        return !(*this == other);
    }

    std::string to_string() const
    {
        std::stringstream ss;
        ss << "<party " << _idx << "[" << _alpha << "]>";
        return ss.str();
    }

    friend std::ostream& operator<<(std::ostream& o, const par<BASE>& p)
    {
        return o << p.to_string();
    }
};

template<base_t BASE>
class parset {
    const size_t _size;
    /* this unique_ptr will forbid copy & assign construction */
    const std::unique_ptr<par<BASE>[]> _pars;
public:
    /* allow implicit construction */
    parset(const std::initializer_list<par<BASE>>& args) :
        _size(args.size()), _pars(new par<BASE>{*args.begin()}) {}

    /* allow moving construction */
    parset(parset&& right) :
        _size(right._size), _pars(std::move(right._pars)) {}

    constexpr const size_t& size() const { return _size; }
    constexpr const par<BASE>& operator[](size_t idx) const { return _pars[idx]; }
};

} /* namespace mpc */
