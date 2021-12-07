#pragma once

#include "context.hpp"

#include <string>
#include <sstream>
#include <ostream>

namespace mpc {

class Context;

class PartyDecl {
    Context& _ctx;
    std::string _name;

    PartyDecl(const PartyDecl&) = delete;
    PartyDecl(PartyDecl&&) = delete;
    PartyDecl& operator=(const PartyDecl&) = delete;
    PartyDecl& operator=(PartyDecl&&) = delete;

    explicit PartyDecl(Context& context, const std::string& name) noexcept :
        _ctx(context), _name(name) {}

public:
    static std::weak_ptr<PartyDecl> new_party(Context& context,
            const std::string& name)
    {
        std::shared_ptr<PartyDecl> p(new PartyDecl(context, name));
        context.register_party(name, p);
        return p;
    }

    std::string name() { return _name; }

    Context& context() { return _ctx; }

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
