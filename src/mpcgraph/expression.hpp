#pragma once

#include "partydecl.hpp"

#include <vector>
#include <memory>

namespace mpc {

class Expression {
    // std::vector<std::weak_ptr<Expression>> oprands;
protected:
    Expression() {}
};

class Literal : public Expression {
    friend Context;
protected:
    Literal() {}
};

class Var : public Expression {
    std::string _name;

protected:
    Var(const std::string& name) : _name(name) {}

public:
    std::string name() { return _name; }
};


class Placeholder : public Var {
    friend Context;

    std::weak_ptr<PartyDecl> _party;
protected:
    Placeholder(const std::string& name,
        const std::weak_ptr<PartyDecl>& party) :
            Var(name), _party(party) {}
};

} /* namespace mpc */
