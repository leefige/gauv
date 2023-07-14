#pragma once

#include <functional>
#include <memory>
#include <sstream>
#include <unordered_map>

#include <iostream>

namespace mpc {

class PartyDecl;

class Secret;
class Constant;
class Share;

class Poly;

class Context {
    std::string _name;

    std::unordered_map<std::string, std::reference_wrapper<PartyDecl>> _parties;
    std::unordered_map<std::string, std::reference_wrapper<Secret>> _secrets;
    std::unordered_map<std::string, std::reference_wrapper<Constant>> _constants;

    std::unordered_map<std::string, std::shared_ptr<Poly>> _polies;
    std::unordered_map<std::string, std::shared_ptr<Share>> _shares;

    /**
     * @brief Construct a new Context object.
     *
     * @param name Name of this context.
     */
    explicit Context(const std::string& name) noexcept : _name(name) {}

    explicit Context() noexcept : Context("context") {}

    template <typename T>
    bool _register_ref_to_context(const std::string& name, T& object,
            std::unordered_map<std::string, std::reference_wrapper<T>>& map)
    {
        if (map.find(name) != map.end()) {
            return false;
        }
        map.insert(std::make_pair(name, std::ref(object)));
        return true;
    }

    template <typename T>
    bool _register_ptr_to_context(const std::string& name, T* object,
            std::unordered_map<std::string, std::shared_ptr<T>>& map)
    {
        if (map.find(name) != map.end()) {
            return false;
        }
        map.insert(std::make_pair(name, object));
        return true;
    }

    Context(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(const Context&) = delete;
    Context& operator=(Context&&) = delete;

public:
    ~Context() {}

    static Context& get_context()
    {
        static Context ctx;
        return ctx;
    }

    /**
     * @brief Register a named party.
     *
     * @param name Name of the party.
     * @param party Reference to the party.
     * @return bool True if seccess, false if the name of this party has been
     * rigistered in this context.
     */
    bool register_party(const std::string& name, PartyDecl& party)
    {
        return _register_ref_to_context(name, party, _parties);
    }

    /**
     * @brief Get the reference to a named party that has been registered
     * to this context.
     *
     * @param name Name of the party.
     * @return PartyDecl& Reference to the named party.
     *
     * @exception std::out_of_range If no party with `name` is present.
     */
    PartyDecl& party(const std::string& name) { return _parties.at(name); }

    /**
     * @brief Register a secret input for some party.
     *
     * @param name Name of the secret.
     * @param type Type of the secret (arithmetic or binary).
     * @param secret Reference to the secret variable to be registered.
     * @return bool True if seccess, false if the name of this secret has been
     * rigistered in this context.
     */
    bool register_secret(const std::string& name, Type* type, Secret& secret)
    {
        return _register_ref_to_context(name, secret, _secrets);
    }

    /**
     * @brief Get the reference to a secret that has been registered
     * to this context.
     *
     * @param name Name of the secret.
     * @return Secret& Reference to the secret.
     *
     * @exception std::out_of_range If no secret with `name` is present.
     */
    Secret& secret(const std::string& name) { return _secrets.at(name); }

    /**
     * @brief Declare a named constant.
     *
     * @param name Name of the constant.
     * @param var Reference to the constant to be registered.
     * @return bool True if seccess, false if the name of this constant has been
     * rigistered in this context.
     */
    bool register_constant(const std::string& name, Constant& var)
    {
        return _register_ref_to_context(name, var, _constants);
    }

    /**
     * @brief Get the reference to a named constant that has been registered
     * to this context.
     *
     * @param name Name of the constant.
     * @return Constant& Reference to the named constant.
     *
     * @exception std::out_of_range If no constant with `name` is present.
     */
    Constant& constant(const std::string& name) { return _constants.at(name); }

    bool register_poly(const std::string& name, Poly* poly)
    {
        return _register_ptr_to_context(name, poly, _polies);
    }

    bool register_share(const std::string& name, Share* share)
    {
        return _register_ptr_to_context(name, share, _shares);;
    }

    size_t n_poly() const { return _polies.size(); }
    size_t n_share() const { return _shares.size(); }

};

} /* namespace mpc */
