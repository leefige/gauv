#pragma once

#include "field.hpp"
#include "party.hpp"
#include "mpc_exceptions.hpp"

#include <sstream>
#include <unordered_map>

namespace mpc {

template<base_t BASE>
class share {
    using F = field<BASE>;
    using P = par<BASE>;

    const P& _party;
    const F _val;

public:
    explicit share(const P& party) noexcept : _party(party), _val(0) {}
    explicit share(const P& party, const F& value) noexcept : _party(party), _val(value) {}

    const P& party() const { return _party; }

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

template<base_t BASE>
class bundle {
    const parset<BASE> _parties;
    const std::vector<share<BASE>> _shares;
    std::unordered_map<size_t, size_t> _mapping;

public:
    explicit bundle(const parset<BASE>& parties, const std::vector<share<BASE>>& shares) :
        _parties(parties), _shares(shares)
    {
        if (_parties.size() != _shares.size()) {
            throw argc_missmatch(_parties.size(), _shares.size());
        }


        for (int i = 0; i < _parties.size(); i++) {
            if (_parties[i] != _shares[i].party()) {
                throw party_missmatch(_parties[i], _shares[i].party());
            }
            _mapping.insert(std::make_pair(_parties[i].idx(), i));
        }
    }

    const parset<BASE>& parties() const { return _parties; }

    const share<BASE>& operator[](const par<BASE>& party) const
    {
        return _shares[_mapping.at(party.idx())];
    }

    bundle<BASE> operator+(const bundle<BASE>& other) const
    {
        if (_parties != other._parties) {
            throw party_missmatch(_parties, other._parties);
        }

        std::vector<share<BASE>> res;
        res.reserve(_shares.size());
        for (const auto& p : _parties) {
            res.push_back((*this)[p] + other[p]);
        }

        return bundle<BASE>(_parties, res);
    }

    bundle<BASE> operator-(const bundle<BASE>& other) const
    {
        if (_parties != other._parties) {
            throw party_missmatch(_parties, other._parties);
        }

        std::vector<share<BASE>> res;
        res.reserve(_shares.size());
        for (const auto& p : _parties) {
            res.push_back((*this)[p] - other[p]);
        }

        return bundle<BASE>(_parties, res);
    }

    bundle<BASE> operator*(const bundle<BASE>& other) const
    {
        if (_parties != other._parties) {
            throw party_missmatch(_parties, other._parties);
        }

        std::vector<share<BASE>> res;
        res.reserve(_shares.size());
        for (const auto& p : _parties) {
            res.push_back((*this)[p] * other[p]);
        }

        return bundle<BASE>(_parties, res);
    }
};

} /* namespace mpc */
