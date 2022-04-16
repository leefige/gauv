#include "../../src/mpcgraph/builtin.hpp"

#include <iostream>

#include <assert.h>

using namespace mpc;
using namespace std;

void test_secret()
{
    Context& ctx = Context::get_context();

    PartyDecl p1(ctx, "p1");
    PartyDecl p2(ctx, "p2");

    Secret x(ctx, "x", p1);
    Secret y(ctx, "y", p2);

    cout << x << " " << y << endl;
    cout << ctx.secret("x") << " " << ctx.secret("y") << endl;
    assert(&x == &(ctx.secret("x")));

    try {
        (void) ctx.secret("bad");
    } catch(const std::out_of_range& e) {
        cout << "Exception caught: ";
        cerr << e.what() << endl;
    }

    try {
        Secret(ctx, "x", p1);
    } catch(const var_redefinition& e) {
        cout << "Exception caught: ";
        cerr << e.what() << endl;
    }
}

void test_constant()
{
    Context& ctx = Context::get_context();

    Constant x(ctx, "x");
    Constant y(ctx, "y");

    cout << x << " " << y << endl;
    cout << ctx.constant("x") << " " << ctx.constant("y") << endl;
    assert(&x == &(ctx.constant("x")));

    try {
        (void) ctx.constant("bad");
    } catch(const std::out_of_range& e) {
        cout << "Exception caught: ";
        cerr << e.what() << endl;
    }

    try {
        Constant(ctx, "x");
    } catch(const var_redefinition& e) {
        cout << "Exception caught: ";
        cerr << e.what() << endl;
    }
}

int main()
{
    test_secret();
    test_constant();
    return 0;
}
