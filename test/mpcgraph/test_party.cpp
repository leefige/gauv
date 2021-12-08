#include "../../src/mpcgraph/context.hpp"
#include "../../src/mpcgraph/partydecl.hpp"

#include <iostream>

#include <assert.h>

using namespace mpc;
using namespace std;

void test_party()
{
    Context ctx = Context::get_context();

    PartyDecl p1(ctx, "p1");
    PartyDecl p2(ctx, "p2");

    // PartyDecl pp(p1);
    // PartyDecl pp = p1;

    cout << p1 << " " << p2 << endl;
    cout << ctx.party("p1") << " " << ctx.party("p2") << endl;
    assert(&p1 == &(ctx.party("p1")));

    try {
        (void) ctx.party("bad");
    } catch(const std::out_of_range& e) {
        cout << "Exception caught: ";
        cerr << e.what() << endl;
    }

    try {
        PartyDecl(ctx, "p1");
    } catch(const party_redefinition& e) {
        cout << "Exception caught: ";
        cerr << e.what() << endl;
    }
}

int main()
{
    test_party();
    return 0;
}
