#include "../../src/mpcgraph/context.hpp"
#include "../../src/mpcgraph/partydecl.hpp"
#include "../../src/mpcgraph/expression.hpp"
#include "../../src/mpcgraph/share.hpp"

#include <iostream>

#include <assert.h>

using namespace mpc;
using namespace std;

void test_share()
{
    Context ctx = Context::get_context();

    PartyDecl p1(ctx, "p1");
    PartyDecl p2(ctx, "p2");

    Secret x(ctx, "x", p1);
    Secret y(ctx, "y", p2);

    Equation e1(Operator::ADD, {&x, &y});
    Equation e2(Operator::MUL, {&y, &x});

    Share s1(ctx, "s1", e1, p1);
    Share s2(ctx, "s1", e2, p2);

    cout << s1 << endl << s2 << endl;
}

int main()
{
    test_share();
    return 0;
}
