#pragma once

#include <ostream>
#include <sstream>

#include <cstdint>

namespace mpc {

using base_t = uint64_t;

template<base_t BASE>
class field {
    base_t _val;
public:
    field() noexcept : _val(0) {}

    /* forbid implicit conversion */
    explicit field(const base_t& val) noexcept : _val(val % BASE) {}
    explicit operator base_t() const { return _val; }

    constexpr base_t base() const { return BASE; }

    field operator+(const field<BASE>& other) const
    {
        return field<BASE>((_val + base_t(other)) % BASE);
    }

    field operator-(const field<BASE>& other) const
    {
        return field<BASE>((_val - base_t(other)) % BASE);
    }

    field operator*(const field<BASE>& other) const
    {
        return field<BASE>((_val * base_t(other)) % BASE);
    }

    const field& operator+=(const field<BASE>& other)
    {
        _val += base_t(other);
        _val %= BASE;
        return *this;
    }

    const field& operator-=(const field<BASE>& other)
    {
        _val -= base_t(other);
        _val %= BASE;
        return *this;
    }

    const field& operator*=(const field<BASE>& other)
    {
        _val *= base_t(other);
        _val %= BASE;
        return *this;
    }

    bool operator==(const field<BASE>& other) const
    {
        return _val == other._val;
    }

    bool operator!=(const field<BASE>& other) const
    {
        return !(*this == other);
    }

    friend std::ostream& operator<<(std::ostream& o, const field<BASE>& f)
    {
        return o << f._val << "_F{" << BASE << "}";
    }
};

} /* namespace mpc */
