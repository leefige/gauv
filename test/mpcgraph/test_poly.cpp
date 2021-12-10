#include "../../src/mpcgraph/context.hpp"
#include "../../src/mpcgraph/partydecl.hpp"
#include "../../src/mpcgraph/expression.hpp"
#include "../../src/mpcgraph/poly.hpp"

#include <iostream>

#include <assert.h>

using namespace mpc;
using namespace std;

void test_poly()
{
    Context ctx = Context::get_context();

    PartyDecl p1(ctx, "p1");
    PartyDecl p2(ctx, "p2");

    Secret x(ctx, "x", p1);
    Secret y(ctx, "y", p2);

    Poly poly1 = Poly::gen_poly(ctx, x, 2);
    Poly poly2 = Poly::gen_poly(ctx, Constant::zero, 2);

    cout << poly1 << endl << poly2 << endl;
}

int main()
{
    test_poly();
    return 0;
}
