#include "../../src/mpcgraph/builtin.hpp"

#include <iostream>

#include <assert.h>

using namespace mpc;
using namespace std;

void test_share()
{
    Context& ctx = Context::get_context();

    PartyDecl p1(ctx, "p1");
    PartyDecl p2(ctx, "p2");

    Secret x(ctx, "x", ArithFieldType::get_arith_field_type(), p1);
    Secret y(ctx, "y", ArithFieldType::get_arith_field_type(), p2);

    Poly& poly1 = Poly::gen_poly(ctx, p1, x, 2);
    Poly& poly2 = Poly::gen_poly(ctx, p2, Constant::zero, 2);

    auto& s1 = poly1.eval(p1);
    auto& s2 = poly1.eval(p2);
    cout << s1 << endl << s2 << endl;

    auto& t1 = poly2.eval(p1);
    auto& t2 = poly2.eval(p2);
    cout << t1 << endl << t2 << endl;

    auto& t1s = t1.transfer(&p1);
    auto& a1 = s1 + t1s;
    auto& a2 = s1 - t1s;
    auto& a3 = s1 * t1s;
    auto& a4 = s1 / t1s;
    auto& a5 = a1 * a2;
    auto& a6 = a5 + a3;
    cout << t1s << endl
        << a1 << endl
        << a2 << endl
        << a3 << endl
        << a4 << endl
        << a5 << endl
        << a6 << endl;

    try {
        a6 + t2;
    } catch (const std::exception& e) {
        cerr << e.what() << endl;
        cout << "Exception caught" << endl;
    }

    cout << endl;

    auto& s2t = s2.transfer(&p2);

    auto& b1 = s2t + t2;
    auto& b2 = a6.transfer(&p2);
    auto& b3 = b2 / b1;
    cout << s2t << endl
        << b1 << endl
        << b2 << endl
        << b3 << endl;

    try {
        a1.transfer(&p1);
    } catch(const std::exception& e) {
        cerr << e.what() << endl;
        cout << "Exception caught" << endl;
    }
}

int main()
{
    test_share();
    return 0;
}
