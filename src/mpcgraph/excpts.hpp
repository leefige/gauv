#pragma once

#include <string>
#include <sstream>
#include <exception>

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

} /* namespace mpc */
