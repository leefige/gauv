#pragma once

#include <algorithm>
#include <vector>

#include "common.hpp"

namespace bgw {
class Context {
    PartyVec _parties;
    size_t _T;

   public:
    Context() {}
    Context(PartyVec& parties, size_t T) : _parties(parties), _T(T) {}

    const PartyVec& parties() const { return _parties; }
    size_t N() const { return _parties.size(); }
    size_t T() const { return _T; }

    void addParty(mpc::PartyDecl* party) { _parties.push_back(party); }
    void setT(size_t T) { _T = T; }
};
}  // namespace bgw
