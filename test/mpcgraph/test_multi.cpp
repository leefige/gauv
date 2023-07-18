#include "../../src/mpcgraph/builtin.hpp"

#include <iostream>

#include <assert.h>

using namespace mpc;
using namespace std;

void test_multi()
{
    Context& ctx = Context::get_context();

    PartyDecl p1(ctx, "p1");
    PartyDecl p2(ctx, "p2");

    Secret x(ctx, "x", p1);
    Secret y(ctx, "y", p2);

    Poly& poly1 = Poly::gen_poly(ctx, p1, x, 2);
    Poly& poly2 = Poly::gen_poly(ctx, p2, Constant::zero, 2);

    auto& s1 = poly1.eval(p1);
    auto& s2 = poly1.eval(p2);
    cout << s1 << endl << s2 << endl;

    auto& t1 = poly2.eval(p1);
    auto& t2 = poly2.eval(p2);
    cout << t1 << endl << t2 << endl;

    auto& t1s = t1.transfer(&p1);
    cout << t1s << endl;

    // scalar multi
    std::stringstream cs;
    cs << "_const_" << std::to_string(3);
    Constant c = Constant(ctx, cs.str());
    auto& a1 = s1.scalarMul(c);
    cout << a1 << endl;
    // cross multi
    auto& a2 = s1 * t1s;
    cout << a2 << endl;
}

int main()
{
    test_multi();
    return 0;
}