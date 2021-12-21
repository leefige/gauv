#include "../../src/mpcgraph/context.hpp"
#include "../../src/mpcgraph/partydecl.hpp"
#include "../../src/mpcgraph/expression.hpp"
#include "../../src/mpcgraph/poly.hpp"
#include "../../src/mpcgraph/share.hpp"

#include <iostream>

#include <vector>

#include <assert.h>

using namespace mpc;
using namespace std;

void test_rand_2t()
{
    Context& ctx = Context::get_context();

    constexpr const size_t SEC = 2;

    PartyDecl p0(ctx, "p0");
    PartyDecl p1(ctx, "p1");
    PartyDecl p2(ctx, "p2");

    std::vector<PartyDecl*> parties{&p0, &p1, &p2};

    std::vector<Share*> transfers;

    for (const auto& i : parties) {
        auto q_i_x = Poly::gen_poly(ctx, Constant::zero, SEC);
        for (const auto& j : parties) {
            auto& s_i_j = q_i_x.eval(*j);
            transfers.push_back(&s_i_j);
            cout << i->name() << " sends to " << j->name() << ": " << s_i_j.name() << endl;
        }
    }

    for (const auto& i : parties) {
        auto q_i_x = Poly::gen_poly(ctx, Constant::zero, SEC);
        for (const auto& j : parties) {
            auto& s_i_j = q_i_x.eval(*j);
            transfers.push_back(&s_i_j);
            cout << i->name() << " sends to " << j->name() << ": " << s_i_j.name() << endl;
        }
    }
}

int main()
{
    test_rand_2t();
    return 0;
}
