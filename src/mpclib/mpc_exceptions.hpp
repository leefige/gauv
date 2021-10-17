#pragma once

#include "field.hpp"
#include "party.hpp"

#include <sstream>

namespace mpc {

template<base_t BASE>
class party_missmatch: public std::exception {
    std::string msg;
public:
    party_missmatch(const par<BASE>& p1, const par<BASE>& p2) noexcept
    {
        std::stringstream ss;
        ss << "Party missmatch: '" << p1 << "' and '" << p2 << "'";
        msg = ss.str();
    }

    virtual const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

} /* namespace mpc */
