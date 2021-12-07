#pragma once

#include <string>
#include <sstream>
#include <ostream>

namespace mpc {

class Context;

class PartyDecl {
    friend Context;

    std::string _name;

    explicit PartyDecl(const std::string& name) : _name(name) {}

public:
    PartyDecl(const PartyDecl&) = delete;
    PartyDecl(PartyDecl&&) = delete;
    PartyDecl& operator=(const PartyDecl&) = delete;
    PartyDecl& operator=(PartyDecl&&) = delete;

    std::string name() { return _name; }

    std::string to_string() const
    {
        std::stringstream ss;
        ss << "<party " << _name << ">";
        return ss.str();
    }

    friend std::ostream& operator<<(std::ostream& o, const PartyDecl& p)
    {
        return o << p.to_string();
    }
};

} /* namespace mpc */
