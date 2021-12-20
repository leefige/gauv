#include "../../src/mpcgraph/context.hpp"
#include "../../src/mpcgraph/partydecl.hpp"
#include "../../src/mpcgraph/expression.hpp"
#include "../../src/mpcgraph/poly.hpp"
#include "../../src/mpcgraph/share.hpp"

#include <iostream>

#include <assert.h>

using namespace mpc;
using namespace std;

void test_share()
{
    Context& ctx = Context::get_context();

    PartyDecl p1(ctx, "p1");
    PartyDecl p2(ctx, "p2");

    Secret x(ctx, "x", p1);
    Secret y(ctx, "y", p2);

    Poly poly1 = Poly::gen_poly(ctx, x, 2);
    Poly poly2 = Poly::gen_poly(ctx, Constant::zero, 2);

    auto s1 = poly1.eval(p1);
    auto s2 = poly1.eval(p2);
    cout << s1 << endl << s2 << endl;

    auto t1 = poly2.eval(p1);
    auto t2 = poly2.eval(p2);
    cout << t1 << endl << t2 << endl;

    auto a1 = s1 + t1;
    auto a2 = s1 - t1;
    auto a3 = s1 * t1;
    auto a4 = s1 / t1;
    auto a5 = a1 * a2;
    auto a6 = a5 + a3;
    cout << a1 << endl
        << a2 << endl
        << a3 << endl
        << a4 << endl
        << a5 << endl
        << a6 << endl;

    try {
        auto bad = a6 + t2;
    } catch (const std::exception& e) {
        cerr << e.what() << endl;
        cout << "Exception caught" << endl;
    }



}

int main()
{
    test_share();
    return 0;
}
