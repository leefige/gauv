#pragma once

#include "partydecl.hpp"
#include "expression.hpp"

#include "excpts.hpp"

#include <memory>
#include <unordered_map>

namespace mpc {

class Context {
    std::string _name;

    std::unordered_map<std::string, std::shared_ptr<PartyDecl>> _parties;
    std::unordered_map<std::string, std::shared_ptr<Secret>> _secrets;
    std::unordered_map<std::string, std::shared_ptr<Constant>> _constants;

public:
    /**
     * @brief Construct a new Context object.
     *
     * @param name Name of this context.
     */
    explicit Context(const std::string& name) : _name(name) {}

    Context() : Context("context") {}

    /**
     * @brief Return a weak ptr copy of a named party; create if not exist.
     *
     * @param name Name of the party.
     * @return Weak ptr of the party.
     */
    std::weak_ptr<PartyDecl> party(const std::string& name)
    {
        if (_parties.find(name) != _parties.end()) {
            return _parties[name];
        }

        std::shared_ptr<PartyDecl> p(new PartyDecl(name));
        _parties.insert(std::make_pair(name, p));
        return p;
    }

    /**
     * @brief Declare a secret input for some party.
     *
     * @param name Name of the secret variable.
     * @param party Weak ptr of the party owning this secret.
     * @return Weak ptr of the declared secret.
     *
     * @exception var_redefinition Trying to declared a secret with a name
     * that has already been declared.
     */
    std::weak_ptr<Secret> declare_secret(const std::string& name,
            const std::weak_ptr<PartyDecl>& party)
    {
        if (_secrets.find(name) != _secrets.end()) {
            throw var_redefinition(name);
        }

        std::shared_ptr<Secret> p(new Secret(name, party));
        _secrets.insert(std::make_pair(name, p));
        return p;
    }

    /**
     * @brief Declare a named constant.
     *
     * @param name Name of the constant.
     * @return Weak ptr of the declared constant.
     *
     * @exception var_redefinition Trying to declared a constant with a name
     * that has already been declared.
     */
    std::weak_ptr<Constant> declare_constant(const std::string& name)
    {
        if (_constants.find(name) != _constants.end()) {
            throw var_redefinition(name);
        }

        std::shared_ptr<Constant> p(new Constant(name));
        _constants.insert(std::make_pair(name, p));
        return p;
    }

    /**
     * @brief Declare an anonymous constant.
     *
     * @return Weak ptr of the declared constant.
     *
     * @exception var_redefinition Trying to declared a constant with a name
     * that has already been declared.
     */
    std::weak_ptr<Constant> declare_constant()
    {
        std::stringstream ss;
        ss << "const_" << _constants.size();
        return declare_constant(ss.str());
    }

};

} /* namespace mpc */
