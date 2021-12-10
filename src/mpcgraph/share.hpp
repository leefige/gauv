#pragma once

#pragma once

#include "expression.hpp"
#include "excpts.hpp"
#include "context.hpp"
#include "partydecl.hpp"

#include <vector>
#include <memory>

namespace mpc {

class Share : public Expression {
    Share(const Share&) = delete;
    Share(Share&&) = delete;
    Share& operator=(const Share&) = delete;
    Share& operator=(Share&&) = delete;

    const PartyDecl& _party;

// protected:
public:
    explicit Share(Context& context,
            const std::string& name,
            const Equation& eqn,
            const PartyDecl& party)
        : Expression(context, name, eqn), _party(party)
    {
        if (!context.register_share(*this)) {
            throw var_redefinition(name);
        }
    }

    virtual ~Share() {}

    virtual std::string to_string() const override
    {
        std::stringstream ss;
        ss << "<share[" << _party.name() << "] "
            << name() << "=" << equation().op() << "(";

        for (auto cit = equation().coprands().cbegin();
                cit != equation().coprands().cend(); cit++) {
            if (cit != equation().coprands().cbegin()) {
                ss << ",";
            }
            ss << (*cit)->to_string();
        }

        ss << ")" << ">";
        return ss.str();
    }
};

} /* namespace mpc */
