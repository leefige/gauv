#include "../../src/mpcgraph/builtin.hpp"

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

    std::vector<std::vector<Share*>> transfers;
    transfers.resize(parties.size());

    for (int i = 0; i < parties.size(); i++) {
        auto& q_i_x = Poly::gen_poly(ctx, *parties[i], Constant::zero, SEC);
        for (int j = 0; j < parties.size(); j++) {
            auto& s_i_j = q_i_x.eval(*parties[j]).transfer(parties[j]);
            transfers[j].push_back(&s_i_j);
            cout << parties[i]->name() << " sends to " << parties[j]->name() << ": " << s_i_j << endl;
        }
    }

    cout << endl;

    for (int i = 0; i < parties.size(); i++) {
        auto recv_queue = transfers[i];
        Share* delta_i = recv_queue.back();
        recv_queue.pop_back();
        while (recv_queue.size() > 0) {
            auto& partial_sum = *delta_i + *(recv_queue.back());
            recv_queue.pop_back();
            delta_i = &partial_sum;
        }
        cout << parties[i]->name() << " yield delta: " << *delta_i << endl;
    }
}

int main()
{
    test_rand_2t();
    return 0;
}
