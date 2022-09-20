#include <assert.h>

#include <iostream>

#include "../../src/backend/builtin.hpp"
#include "../../src/mpcgraph/builtin.hpp"

using namespace mpc;
using namespace std;

void test_graph_bgw()
{
    Context& ctx = Context::get_context();

    constexpr const size_t SEC = 2;
    constexpr const size_t N = 2 * SEC + 1;

    std::vector<PartyDecl*> parties;
    std::vector<Secret*> secrets;
    for (int i = 0; i < N; i++) {
        auto party = new PartyDecl(ctx, "p" + to_string(i));
        auto secret = new Secret(ctx, "x" + to_string(i), *party);
        parties.push_back(party);
        secrets.push_back(secret);
    }
    for (int i = 0; i < SEC; i++) parties[i]->set_corrupted();

    std::vector<std::vector<Share*>> transfers;
    transfers.resize(parties.size());

    for (int i = 0; i < parties.size(); i++) {
        auto& q_i_x = Poly::gen_poly(ctx, *parties[i], *secrets[i], SEC);
        for (int j = 0; j < parties.size(); j++) {
            auto& s_i_j = q_i_x.eval(*parties[j]).transfer(*parties[j]);
            transfers[j].push_back(&s_i_j);
            cout << parties[i]->name() << " sends to " << parties[j]->name()
                 << ": " << s_i_j << endl;
        }
    }

    cout << endl;

    Graph graph;
    std::vector<Expression*> deltas;
    for (int i = 0; i < parties.size(); i++) {
        auto recv_queue = transfers[i];
        Share* delta_i = recv_queue.back();
        recv_queue.pop_back();
        while (recv_queue.size() > 0) {
            auto& partial_sum = *delta_i + *(recv_queue.back());
            recv_queue.pop_back();
            delta_i = &partial_sum;
        }
        deltas.push_back(&(delta_i->transfer(*parties[0])));
    }
    for (int i = 0; i < parties.size(); i++) {
        Share* subgraph_i = &Share::reconstruct(deltas, *parties[i]);
        cout << parties[i]->name() << " yield subgraph: " << *subgraph_i
             << endl;
        graph.importFrontend(subgraph_i);
    }
    graph.initOutputNodes();
    graph.initSearchState();
    cout << endl << "Init graph:" << endl << graph << endl;
    bool proved = graph.tryProving();
    cout << endl << "Proved? " << std::boolalpha << proved << endl;
    cout << "Result graph:" << endl << graph << endl;
    cout << endl << "Transform history:" << endl;
    for (auto& pair : graph.transformTape) {
        cout << pair.first->getName() << " " << Graph::to_string(pair.second)
             << endl;
    }

    for (auto p : parties) delete p;
    for (auto s : secrets) delete s;
}

int main()
{
    test_graph_bgw();
    return 0;
}
