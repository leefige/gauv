#pragma once

#include "partydecl.hpp"
#include "context.hpp"

#include <vector>
#include <memory>

namespace mpc {

class Expression {
    Context& _ctx;

protected:
    Expression(Context& context) noexcept : _ctx(context) {}

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
    Literal(Context& context) noexcept : Expression(context) {}

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

    std::string name() { return __name; }
};

class Constant : public Placeholder {
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
    explicit Constant(Context& context, const std::string& name) noexcept :
        Placeholder(context, name) {}

public:
    static std::weak_ptr<Constant> new_constant(Context& context,
            const std::string& name)
    {
        std::shared_ptr<Constant> p(new Constant(context, name));
        context.register_constant(name, p);
        return p;
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
    Secret(Context& context, const std::string& name,
        const std::weak_ptr<PartyDecl>& party) :
            Placeholder(context, name), __party(party)
    {
        auto pp = party.lock();
        if (!pp) {
            throw std::runtime_error("PartyDecl released");
        }
        _party_name = pp->name();
    }

public:
    static std::weak_ptr<Secret> new_secret(Context& context,
            const std::string& name, const std::weak_ptr<PartyDecl>& party)
    {
        std::shared_ptr<Secret> p(new Secret(context, name, party));
        context.register_secret(name, p);
        return p;
    }

    virtual ~Secret() {}

    virtual std::string to_string() const override
    {
        std::stringstream ss;
        ss << "<secret[" << _party_name << "] " << __name << ">";
        return ss.str();
    }
};

} /* namespace mpc */
