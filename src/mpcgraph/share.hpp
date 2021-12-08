#pragma once

#pragma once

#include "expression.hpp"
#include "partydecl.hpp"
#include "context.hpp"

#include <vector>
#include <memory>

namespace mpc {

class Share : public Expression {
    Share(const Share&) = delete;
    Share(Share&&) = delete;
    Share& operator=(const Share&) = delete;
    Share& operator=(Share&&) = delete;

    const PartyDecl& _party;

protected:
    explicit Share(Context& context) noexcept : Expression(context) {}

public:
    virtual ~Expression() {}

    virtual std::string to_string() const = 0;

    Context& context() { return _ctx; }

    friend std::ostream& operator<<(std::ostream& o, const Expression& p)
    {
        return o << p.to_string();
    }
};

} /* namespace mpc */
