#pragma once

#include "context.hpp"
#include "excpts.hpp"

#include <string>
#include <sstream>
#include <ostream>

namespace mpc {

class Context;

class PartyDecl {
    Context& _ctx;
    const std::string _name;

    PartyDecl(const PartyDecl&) = delete;
    PartyDecl(PartyDecl&&) = delete;
    PartyDecl& operator=(const PartyDecl&) = delete;
    PartyDecl& operator=(PartyDecl&&) = delete;

public:
    /**
     * @brief Construct a new PartyDecl object.
     *
     * @param context Reference to the context of this object.
     * @param name Name of this party.
     *
     * @exception party_redefinition The name of this party has been
     * rigistered in this context.
     */
    explicit PartyDecl(Context& context, const std::string& name)
            : _ctx(context), _name(name)
    {
        if (!context.register_party(name, *this)) {
            throw party_redefinition(name);
        }
    }

    /**
     * @brief Get the name.
     *
     * @return std::string Name of this party.
     */
    std::string name() const { return _name; }

    /**
     * @brief Get the context.
     *
     * @return Context& Context of this party.
     */
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
