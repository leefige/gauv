#pragma once

#include "field.hpp"

#include <vector>
#include <functional>
#include <initializer_list>
#include <ostream>
#include <sstream>

#include <cstdint>

namespace mpc {

using size_t = uint32_t;

template<base_t BASE>
class par {
    const size_t _idx;
    const field<BASE> _alpha;

    par(const par<BASE>&) = delete;
    par(par<BASE>&&) = delete;
    par<BASE>& operator=(const par<BASE>&) = delete;
    par<BASE>& operator=(par<BASE>&&) = delete;

public:
    explicit par(const size_t& idx, const base_t& alpha) noexcept : _idx(idx), _alpha(alpha) {}
    explicit par(const size_t& idx, const field<BASE>& alpha) noexcept : _idx(idx), _alpha(alpha) {}

    // static par<BASE> decl_party(const size_t& idx, const base_t& alpha) noexcept { return par(idx, alpha); }
    // static par<BASE> decl_party(const size_t& idx, const field<BASE>& alpha) noexcept { return par(idx, alpha); }

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
        ss << "<party " << _idx << ": " << _alpha << ">";
        return ss.str();
    }

    friend std::ostream& operator<<(std::ostream& o, const par<BASE>& p)
    {
        return o << p.to_string();
    }
};

template<base_t BASE>
class parset {
    using P = par<BASE>;
    using P_wrap = std::reference_wrapper<P>;

    const size_t _size;
    std::vector<P_wrap> _pars;

    parset<BASE>& operator=(const parset<BASE>&) = delete;
    parset<BASE>& operator=(parset&&) = delete;

public:
    /* allow implicit construction */
    parset(const std::initializer_list<P_wrap>& args) noexcept :
        _size(args.size())
    {
        _pars.reserve(_size);
        for (const auto& p : args) {
            _pars.push_back(p);
        }
    }

    /* allow moving construction */
    parset(parset&& right) noexcept :
        _size(right._size), _pars(std::move(right._pars)) {}

    parset(const parset<BASE>& other) noexcept :
        _size(other._size), _pars(other._pars) {}

    bool operator!=(const parset<BASE>& other) const
    {
        if (_size != other._size) {
            return true;
        }

        for (int i = 0; i < _size; i++) {
            if (_pars[i].get() != other._pars[i].get()) {
                return true;
            }
        }

        return false;
    }

    bool operator==(const parset<BASE>& other) const { return !(*this != other); }

    constexpr const size_t& size() const { return _size; }
    constexpr P& operator[](size_t idx) const { return _pars[idx]; }

    constexpr const auto begin() const { return _pars.begin(); }
    constexpr const auto end() const { return _pars.end(); }

    std::string to_string() const
    {
        std::stringstream ss;
        ss << "<party_set [";
        for (const auto& p : _pars) {
            ss << p;
        }
        ss << "]>";
        return ss.str();
    }

    friend std::ostream& operator<<(std::ostream& o, const parset<BASE>& p)
    {
        return o << p.to_string();
    }
};

} /* namespace mpc */
