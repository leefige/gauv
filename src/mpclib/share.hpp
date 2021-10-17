#pragma once

#include "field.hpp"
#include "party.hpp"
#include "mpc_exceptions.hpp"

#include <sstream>

namespace mpc {

template<base_t BASE>
class share {
    using F = field<BASE>;
    using P = par<BASE>;

    /* party designed to be const ref. maybe useful??? */
    const P& _party;
    const F _val;

public:
    explicit share(const P& party) : _party(party), _val(0) {}
    explicit share(const P& party, const F& value) : _party(party), _val(value) {}

    share operator+(const share<BASE>& other) const
    {
        if (_party != other._party) {
            throw party_missmatch(_party, other._party);
        }

        return share(_party, _val + other._val);
    }

    share operator-(const share<BASE>& other) const
    {
        if (_party != other._party) {
            throw party_missmatch(_party, other._party);
        }

        return share(_party, _val - other._val);
    }

    share operator*(const share<BASE>& other) const
    {
        if (_party != other._party) {
            throw party_missmatch(_party, other._party);
        }

        return share(_party, _val * other._val);
    }

    std::string to_string() const
    {
        std::stringstream ss;
        ss << "<share[" << _party << "] " << _val << ">";
        return ss.str();
    }

    friend std::ostream& operator<<(std::ostream& o, const share<BASE>& s)
    {
        return o << s.to_string();
    }
};

} /* namespace mpc */
