#pragma once

#include <string>
#include <sstream>
#include <exception>

#include "type.hpp"

namespace mpc {

class var_redefinition : public std::exception {
    std::string msg;
public:
    var_redefinition(const std::string& name) noexcept
    {
        std::stringstream ss;
        ss << "Variable '" << name << "' has been declared in this context";
        msg = ss.str();
    }

    virtual const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

class party_redefinition : public std::exception {
    std::string msg;
public:
    party_redefinition(const std::string& name) noexcept
    {
        std::stringstream ss;
        ss << "Party '" << name << "' has been declared in this context";
        msg = ss.str();
    }

    virtual const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

class party_undeclared : public std::exception {
    std::string msg;
public:
    party_undeclared(const std::string& name) noexcept
    {
        std::stringstream ss;
        ss << "Party '" << name << "' does not exist in this context";
        msg = ss.str();
    }

    virtual const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

class party_mismatch : public std::exception {
    std::string msg;
public:
    party_mismatch(const PartyDecl& want, const PartyDecl& get) noexcept
    {
        std::stringstream ss;
        ss << "Wants " << want << ", gets " << get;
        msg = ss.str();
    }

    virtual const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

class party_duplicated : public std::exception {
    std::string msg;
public:
    party_duplicated(const PartyDecl& party) noexcept
    {
        std::stringstream ss;
        ss << "Wants a party other than " << party;
        msg = ss.str();
    }

    virtual const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

class type_mismatch : public std::exception {
    std::string msg;
public:
    type_mismatch(Type* dest_type, Type* got_type) noexcept
    {
        std::stringstream ss;
        ss << "Wants a type" << dest_type->to_string() << ", but got " << got_type->to_string();
        msg = ss.str();
    }

    virtual const char* what() const noexcept override
    {
        return msg.c_str();
    }    
}

} /* namespace mpc */
