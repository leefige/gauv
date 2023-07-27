#pragma once

#include <initializer_list>
#include <memory>
#include <vector>

#include "context.hpp"
#include "operator.hpp"
#include "partydecl.hpp"
#include "type.hpp"

namespace mpc {

class Equation {
    const Operator _op;
    std::vector<Expression*> _oprands;

    // Equation(const Equation&) = delete;
    // Equation(Equation&&) = delete;
    // Equation& operator=(const Equation&) = delete;
    // Equation& operator=(Equation&&) = delete;

   public:
    std::vector<void*> params;

    static const Equation nulleqn;

    // TODO: ensure oprands in the same context
    // TODO: type checking
    explicit Equation(const Operator& op,
                      const std::vector<Expression*>& oprands) noexcept
        : _op(op), _oprands(oprands) {}

    explicit operator bool() const noexcept { return this != &nulleqn; }

    const Operator& op() const { return _op; }
    std::vector<Expression*>& oprands() { return _oprands; }
    const std::vector<Expression*>& coprands() const { return _oprands; }
};

inline const Equation Equation::nulleqn(Operator::NONE, {});

class Expression {
    Context& _ctx;
    std::string _name;
    Type* _type;
    Equation _eqn;
    const PartyDecl* _party;

   protected:
    explicit Expression(Context& context, const std::string& name, Type* type, const Equation& eqn, const PartyDecl* party = nullptr) noexcept
        : _ctx(context), _name(name), _type(type), _eqn(eqn), _party(party) {}

    explicit Expression(Context& context, const std::string& name, Type* type, const PartyDecl* party = nullptr) noexcept
        : Expression(context, name, type, Equation::nulleqn, party) {}

   public:
    virtual ~Expression() {}

    virtual std::string to_string() const = 0;

    Context& context() { return _ctx; }

    std::string name() const { return _name; }

    Type* type() { return _type; }

    Equation& equation() { return _eqn; }
    const Equation& cequation() const { return _eqn; }

    const PartyDecl* party() const { return _party; }

    friend std::ostream& operator<<(std::ostream& o, const Expression& p) {
        return o << p.to_string();
    }
};

class Literal : public Expression {
    // Xingyu: 这个 Literal 似乎没用？是不是它本来的设计意图被 Constant 取代了？
   protected:
    explicit Literal(Context& context, const std::string& name) noexcept
        : Expression(context, name,  ArithFieldType::get_arith_field_type(), Equation(Operator::INPUT, {})) {}

   public:
    virtual ~Literal() {}
};

class Placeholder : public Expression {
   protected:
    explicit Placeholder(Context& context, const std::string& name, Type* type, const PartyDecl* party = nullptr) noexcept
        : Expression(context, name, type, Equation(Operator::INPUT, {}), party) {}

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
     * registered in this context.
     * 
     * @note So far we assume this is an arithmetic (integer) constant.
     */
    explicit Constant(Context& context, const std::string& name)
        : Placeholder(context, name, ArithFieldType::get_arith_field_type()) {
        if (!context.register_constant(name, *this)) {
            throw var_redefinition(name);
        }
    }

    static Constant zero;
    static Constant build_constant(Context& ctx, std::string val) {
        std::stringstream cs;
        cs << "const_" << val;
        return Constant(ctx, cs.str());
    }

    virtual ~Constant() {}

    virtual std::string to_string() const override {
        std::stringstream ss;
        ss << "<const " << name() << ">";
        return ss.str();
    }
};

inline Constant Constant::zero(Context::get_context(), "_const_zero");

class Secret : public Placeholder {
    // TODO(Xingyu): "secret" is not a suitable name for a kind of expression, because it is not "context-free", i.e., it depends on how this expression is used.
    // But actually, this expression cannot only be used as a secret of a Shamir sharing, but also possible to be added or substracted.
    // Maybe this is more like "input"?
    
    Secret(const Secret&) = delete;
    Secret(Secret&&) = delete;
    Secret& operator=(const Secret&) = delete;
    Secret& operator=(Secret&&) = delete;

   public:
    /**
     * @brief Construct a new secret of some party.
     *
     * @param name Name of the secret.
     * @param party Const reference to the party.
     *
     * @exception var_redefinition The name of this secret has been
     * registered in this context.
     */
    explicit Secret(Context& context, const std::string& name, Type* type, const PartyDecl* party)
        : Placeholder(context, name, type, party) {
        if (!context.register_secret(name, *this)) {
            throw var_redefinition(name);
        }
    }

    virtual ~Secret() {}

    virtual std::string to_string() const override {
        std::stringstream ss;
        ss << "<secret[" << party()->name() << "] " << name() << ">";
        return ss.str();
    }
};

class Randomness : public Placeholder {
    Randomness(const Randomness&) = delete;
    Randomness(Randomness&&) = delete;
    Randomness& operator=(const Randomness&) = delete;
    Randomness& operator=(Randomness&&) = delete;

   public:
    /**
     * @brief Construct a new randomness of some party.
     *
     * @param name Name of the randomness.
     * @param party Const reference to the party.
     *
     * @exception var_redefinition The name of this randomness has been
     * registered in this context.
     */
    explicit Randomness(Context& context, const std::string& name, Type* type, const PartyDecl* party)
        : Placeholder(context, name, type, party) {
        if (!context.register_randomness(name, this)) {
            throw var_redefinition(name);
        }
    }

    virtual ~Randomness() {}

    virtual std::string to_string() const override {
        std::stringstream ss;
        ss << "<randomness[" << party()->name() << "] " << name() << ">";
        return ss.str();
    }
};

class TypeCast : public Expression {
    TypeCast(const TypeCast&) = delete;
    TypeCast(TypeCast&&) = delete;
    TypeCast& operator=(const TypeCast&) = delete;
    TypeCast& operator=(TypeCast&&) = delete;

public:
    explicit TypeCast(Context& context, const std::string& name, Type* type, Expression* expr, const PartyDecl* party)
        : Expression(context, name, type, Equation(Operator::TYPECAST, {expr}), party) {}

    virtual ~TypeCast() {}

    virtual std::string to_string() const override {
        std::stringstream ss;
        ss << "<typecast " << name() << ">";
        return ss.str();
    }
};

} /* namespace mpc */
