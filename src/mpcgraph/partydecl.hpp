#pragma once

#include <spdlog/spdlog.h>

#include <iostream>
#include <sstream>
#include <string>

#include "context.hpp"
#include "excepts.hpp"

namespace mpc {

class Context;

class PartyDecl {
    Context& _ctx;
    const std::string _name;
    bool _corrupted = false; // FIXME: now this field is obsoleted, replaced by the a set of corrupted parties in the Graph class
    int _id;

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
     * registered in this context.
     */
    explicit PartyDecl(Context& context, const std::string& name)
        : _ctx(context), _name(name) {
        if (!context.register_party(name, *this)) {
            throw party_redefinition(name);
        }

        static int id_cnt = 0;
        _id = id_cnt++;

        spdlog::debug("declare a party {}", _id);
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

    int id() const { return _id; }

    std::string to_string() const {
        std::stringstream ss;
        ss << "<party " << _name << ">";
        return ss.str();
    }

    bool is_corrupted() const {
        throw "obsoleted";
        return _corrupted;
    }
    bool is_honest() const {
        throw "obsoleted";
        return !_corrupted;
    }
    void set_corrupted() {
        throw "obsoleted";
        _corrupted = true;
    }
    void set_honest() {
        throw "obsoleted";
        _corrupted = false;
    }

    friend std::ostream& operator<<(std::ostream& o, PartyDecl* p) {
        return o << p->to_string();
    }

    friend bool operator==(const PartyDecl& a, const PartyDecl& b) {
        return &a == &b;
    }

    friend bool operator!=(const PartyDecl& a, const PartyDecl& b) {
        return !(a == b);
    }
};

} /* namespace mpc */
