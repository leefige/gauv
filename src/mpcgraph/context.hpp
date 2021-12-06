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
    std::unordered_map<std::string, std::shared_ptr<Placeholder>> _secrets;
    std::unordered_map<std::string, std::shared_ptr<Var>> _vars;

public:
    Context(const std::string& name) : _name(name) {}

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
     * @exception var_redefinition Trying to declared a secret with a name \
     * that has already been declared.
     */
    std::weak_ptr<Placeholder> declare_secret(const std::string& name,
            const std::weak_ptr<PartyDecl>& party)
    {
        if (_secrets.find(name) != _secrets.end()) {
            throw var_redefinition(name);
        }

        std::shared_ptr<Placeholder> p(new Placeholder(name, party));
        _secrets.insert(std::make_pair(name, p));
        return p;
    }

};

} /* namespace mpc */
