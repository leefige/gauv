#pragma once

#include "context.hpp"
#include "operator.hpp"
#include "partydecl.hpp"

#include <vector>
#include <memory>

namespace mpc {

class Expression {
    Context& _ctx;
    const Equation _eqn;
    std::string _name;

protected:

    explicit Expression(Context& context, const std::string& name,
        const Equation& eqn) noexcept
        : _ctx(context), _eqn(eqn) {}

    explicit Expression(Context& context, const std::string& name) noexcept
        : Expression(context, name, Equation::nulleqn) {}

public:
    virtual ~Expression() {}

    virtual std::string to_string() const = 0;

    Context& context() { return _ctx; }

    std::string name() const { return _name; }

    const Equation& equation() const { return _eqn; }

    friend std::ostream& operator<<(std::ostream& o, const Expression& p)
    {
        return o << p.to_string();
    }
};

class Literal : public Expression {
protected:
    explicit Literal(Context& context, const std::string& name) noexcept
        : Expression(context, name) {}

public:
    virtual ~Literal() {}
};

class Placeholder : public Expression {
protected:

    explicit Placeholder(Context& context, const std::string& name) noexcept
        : Expression(context, name) {}

public:
    virtual ~Placeholder() {}

};

class Constant : public Placeholder {
    Constant(const Constant&) = delete;
    Constant(Constant&&) = delete;
    Constant& operator=(const Constant&) = delete;
    Constant& operator=(Constant&&) = delete;

public:
    /**
     * @brief Construct a placeholder for a constant.
     *
     * @param name Name of the constant.
     *
     * @exception var_redefinition The name of this constant has been
     * rigistered in this context.
     */
    explicit Constant(Context& context, const std::string& name)
            : Placeholder(context, name)
    {
        if (!context.register_constant(name, *this)) {
            throw var_redefinition(name);
        }
    }

    virtual ~Constant() {}

    virtual std::string to_string() const override
    {
        std::stringstream ss;
        ss << "<const " << name() << ">";
        return ss.str();
    }
};


class Secret : public Placeholder {
    Secret(const Secret&) = delete;
    Secret(Secret&&) = delete;
    Secret& operator=(const Secret&) = delete;
    Secret& operator=(Secret&&) = delete;

    const PartyDecl& _party;

public:
    /**
     * @brief Construct a new secret of some party.
     *
     * @param name Name of the secret.
     * @param party Const reference to the party.
     *
     * @exception var_redefinition The name of this secret has been
     * rigistered in this context.
     */
    explicit Secret(Context& context, const std::string& name,
            const PartyDecl& party)
            : Placeholder(context, name), _party(party)
    {
        if (!context.register_secret(name, *this)) {
            throw var_redefinition(name);
        }
    }

    virtual ~Secret() {}

    /**
     * @brief Get the party of this secret.
     *
     * @return const PartyDecl& Const reference to the party.
     */
    const PartyDecl& party() const { return _party; }

    virtual std::string to_string() const override
    {
        std::stringstream ss;
        ss << "<secret[" << _party.name() << "] " << name() << ">";
        return ss.str();
    }
};

} /* namespace mpc */
