#include "../../src/mpcgraph/partydecl.hpp"
#include "../../src/mpcgraph/context.hpp"

#include <iostream>

#include <assert.h>

using namespace mpc;
using namespace std;

int main()
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
    return 0;
}
