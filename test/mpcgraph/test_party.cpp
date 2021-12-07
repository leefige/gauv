#include "../../src/mpcgraph/partydecl.hpp"
#include "../../src/mpcgraph/context.hpp"

#include <iostream>

#include <assert.h>

using namespace mpc;
using namespace std;

void test1()
{
    Context ctx("ctx");
    // PartyDecl party1("p1");
    auto p1 = ctx.party("p1");
    auto p2 = ctx.party("p2");

    auto x = ctx.declare_secret("x", p1);
    auto y = ctx.declare_secret("y", p2);

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
}

void test_delete()
{
    auto ctx = new Context();
    // PartyDecl party1("p1");
    auto p1 = ctx->party("p1");
    auto p2 = ctx->party("p2");

    auto x = ctx->declare_secret("x", p1);
    auto y = ctx->declare_secret("y", p2);

    {
        auto sp1 = p1.lock();
        assert(sp1);
        auto sp2 = p2.lock();
        assert(sp2);
    }

    delete ctx;
    {
        auto sp1 = p1.lock();
        assert(!sp1);
        auto sp2 = p2.lock();
        assert(!sp2);

        try {
            auto bad = ctx->declare_secret("bad", p1);
        } catch(const runtime_error& e)
        {
            cerr << e.what() << endl;
            cout << "Exception caught" << endl;
        }
    }
}

int main()
{
    test1();
    test_delete();
    return 0;
}
