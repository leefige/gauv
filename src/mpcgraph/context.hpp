#pragma once

#include "excpts.hpp"

#include <memory>
#include <unordered_map>

namespace mpc {

class PartyDecl;
class Secret;
class Constant;

class Context {
    std::string _name;

    std::unordered_map<std::string, std::shared_ptr<PartyDecl>> _parties;
    std::unordered_map<std::string, std::shared_ptr<Secret>> _secrets;
    std::unordered_map<std::string, std::shared_ptr<Constant>> _constants;

    /**
     * @brief Construct a new Context object.
     *
     * @param name Name of this context.
     */
    explicit Context(const std::string& name) : _name(name) {}

    Context() : Context("context") {}

public:
    static Context& get_context()
    {
        static Context ctx;
        return ctx;
    }

    /**
     * @brief Register a named party.
     *
     * @param name Name of the party.
     * @param party Shared ptr of the party.
     * @return Void.
     *
     * @exception std::runtime_error `party` is null ptr.
     * @exception party_redefinition The name of this party has been
     * rigistered in this context.
     */
    void register_party(const std::string& name,
            const std::shared_ptr<PartyDecl>& party)
    {
        if (!party) {
            throw std::runtime_error("Null PartyDecl pointer");
        }

        if (_parties.find(name) != _parties.end()) {
            throw party_redefinition(name);
        }

        _parties.insert(std::make_pair(name, party));
    }

    std::optional<std::weak_ptr<PartyDecl>> party(const std::string& name)
    {
        auto it = _parties.find(name);
        if (it == _parties.end()) {
            return std::nullopt;
        } else {
            return std::weak_ptr<PartyDecl>(it->second);
        }
    }

    /**
     * @brief Register a secret input for some party.
     *
     * @param name Name of the secret.
     * @param secret Shared ptr of the secret variable to be registered.
     * @return Void.
     *
     * @exception std::runtime_error `secret` is null ptr.
     * @exception var_redefinition Trying to declared a secret with a name
     * that has already been declared.
     */
    void register_secret(const std::string& name,
            const std::shared_ptr<Secret>& secret)
    {
        if (!secret) {
            throw std::runtime_error("Null Secret pointer");
        }

        if (_secrets.find(name) != _secrets.end()) {
            throw var_redefinition(name);
        }

        _secrets.insert(std::make_pair(name, secret));
    }

    std::optional<std::weak_ptr<Secret>> secret(const std::string& name)
    {
        auto it = _secrets.find(name);
        if (it == _secrets.end()) {
            return std::nullopt;
        } else {
            return std::weak_ptr<Secret>(it->second);
        }
    }

    /**
     * @brief Declare a named constant.
     *
     * @param name Name of the constant.
     * @param var Shared ptr of the constant to be registered.
     * @return Void.
     *
     * @exception std::runtime_error `var` is null ptr.
     * @exception var_redefinition Trying to declared a constant with a name
     * that has already been declared.
     */
    void register_constant(const std::string& name,
            std::shared_ptr<Constant> var)
    {
        if (!var) {
            throw std::runtime_error("Null Constant pointer");
        }

        if (_constants.find(name) != _constants.end()) {
            throw var_redefinition(name);
        }

        _constants.insert(std::make_pair(name, var));
    }

    std::optional<std::weak_ptr<Constant>> constant(const std::string& name)
    {
        auto it = _constants.find(name);
        if (it == _constants.end()) {
            return std::nullopt;
        } else {
            return std::weak_ptr<Constant>(it->second);
        }
    }
};

} /* namespace mpc */
