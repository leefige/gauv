#pragma once

#include "partydecl.hpp"

#include <vector>
#include <memory>

namespace mpc {

class Expression {
    // std::vector<std::weak_ptr<Expression>> oprands;
protected:
    Expression() {}

public:
    virtual ~Expression() {}

    virtual std::string to_string() const = 0;

    friend std::ostream& operator<<(std::ostream& o, const Expression& p)
    {
        return o << p.to_string();
    }
};

class Literal : public Expression {
    friend Context;

protected:
    Literal() {}

public:
    virtual ~Literal() {}
};

class Placeholder : public Expression {
protected:
    std::string __name;

    Placeholder(const std::string& name) : __name(name) {}

public:
    virtual ~Placeholder() {}

    std::string name() { return __name; }
};

class Constant : public Placeholder {
    friend Context;

    Constant(const Constant&) = delete;
    Constant(Constant&&) = delete;
    Constant& operator=(const Constant&) = delete;
    Constant& operator=(Constant&&) = delete;

protected:
    /**
     * @brief Construct a placeholder for a constant.
     *
     * @param name Name of the constant.
     */
    Constant(const std::string& name) : Placeholder(name) {}

public:
    virtual ~Constant() {}

    virtual std::string to_string() const override
    {
        std::stringstream ss;
        ss << "<const " << __name << ">";
        return ss.str();
    }
};


class Secret : public Placeholder {
    friend Context;

    std::string _party_name;

    Secret(const Secret&) = delete;
    Secret(Secret&&) = delete;
    Secret& operator=(const Secret&) = delete;
    Secret& operator=(Secret&&) = delete;

protected:
    std::weak_ptr<PartyDecl> __party;

    /**
     * @brief Construct a new secret of some party.
     *
     * @param name Name of the secret.
     * @param party Weak ptr to the party.
     *
     * @exception std::runtime_error `party` is released.
     */
    Secret(const std::string& name,
        const std::weak_ptr<PartyDecl>& party) :
            Placeholder(name), __party(party)
    {
        auto pp = party.lock();
        if (!pp) {
            throw std::runtime_error("PartyDecl released");
        }
        _party_name = pp->name();
    }

public:
    virtual ~Secret() {}

    virtual std::string to_string() const override
    {
        std::stringstream ss;
        ss << "<secret[" << _party_name << "] " << __name << ">";
        return ss.str();
    }
};

} /* namespace mpc */
