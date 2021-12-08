#include "../../src/mpcgraph/context.hpp"
#include "../../src/mpcgraph/partydecl.hpp"
#include "../../src/mpcgraph/expression.hpp"

#include <iostream>

#include <assert.h>

using namespace mpc;
using namespace std;

void test1()
{
    Context ctx = Context::get_context();
    // PartyDecl party1("p1");
    auto p1 = PartyDecl::new_party(ctx, "p1");
    auto p2 = PartyDecl::new_party(ctx, "p2");

    auto x = Secret::new_secret(ctx, "x", p1);
    auto y = Secret::new_secret(ctx, "y", p2);

    auto c = Constant::new_constant(ctx, "c");
    auto cx = Constant::new_constant(ctx, "x");

    {
        auto sp1 = p1.lock();
        assert(sp1);
        auto sp2 = p2.lock();
        assert(sp2);

        cout << *sp1 << " " << *sp2 << endl;
    }

    {
        auto xp = x.lock();
        assert(xp);
        auto yp = y.lock();
        assert(yp);

        cout << *xp << " " << *yp << endl;
    }

    {
        auto cp = c.lock();
        assert(cp);
        auto cxp = cx.lock();
        assert(cxp);

        cout << *cp << " " << *cxp << endl;
    }

    {
        auto sp1 = ctx.party("p1").value().lock();
        assert(sp1);
        auto sp2 = ctx.party("p2").value().lock();
        assert(sp2);
        cout << *sp1 << " " << *sp2 << endl;
    }

    {
        auto xp = ctx.secret("x").value().lock();
        assert(xp);
        auto yp = ctx.secret("y").value().lock();
        assert(yp);

        auto bad = ctx.secret("bad");
        assert(!bad.has_value());

        cout << *xp << " " << *yp << endl;
    }

}

int main()
{
    test1();
    return 0;
}
