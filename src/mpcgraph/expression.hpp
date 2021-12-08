#pragma once

#include "partydecl.hpp"
#include "context.hpp"

#include <vector>
#include <memory>

namespace mpc {

class Expression {
    Context& _ctx;

protected:
    explicit Expression(Context& context) noexcept : _ctx(context) {}

public:
    virtual ~Expression() {}

    virtual std::string to_string() const = 0;

    Context& context() { return _ctx; }

    friend std::ostream& operator<<(std::ostream& o, const Expression& p)
    {
        return o << p.to_string();
    }
};

class Literal : public Expression {
protected:
    explicit Literal(Context& context) noexcept : Expression(context) {}

public:
    virtual ~Literal() {}
};

class Placeholder : public Expression {
protected:
    std::string __name;

    explicit Placeholder(Context& context, const std::string& name) noexcept :
        Expression(context), __name(name) {}

public:
    virtual ~Placeholder() {}

    std::string name() const { return __name; }
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
        ss << "<const " << __name << ">";
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
        ss << "<secret[" << _party.name() << "] " << __name << ">";
        return ss.str();
    }
};

} /* namespace mpc */
