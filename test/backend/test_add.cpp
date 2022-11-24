#include <assert.h>

#include <cstdlib>
#include <iostream>

#include "../../src/backend/builtin.hpp"
#include "../../src/mpcgraph/builtin.hpp"

using namespace mpc;
using namespace std;

void test_bgw_add(size_t I, size_t T, size_t N, int verbose = 1) {
    Context& ctx = Context::get_context();

    std::vector<PartyDecl*> parties;
    std::vector<Secret*> secrets;
    for (int i = 0; i < N; i++) {
        auto party = new PartyDecl(ctx, "p" + to_string(i));
        auto secret = new Secret(ctx, "x" + to_string(i), *party);
        parties.push_back(party);
        secrets.push_back(secret);
    }
    for (int i = 0; i < I; i++) parties[i]->set_corrupted();

    std::vector<std::vector<Share*>> transfers;
    transfers.resize(parties.size());

    for (int i = 0; i < parties.size(); i++) {
        auto& q_i_x = Poly::gen_poly(ctx, *parties[i], *secrets[i], T);
        for (int j = 0; j < parties.size(); j++) {
            auto& s_i_j = q_i_x.eval(*parties[j]);
            if (i != j) {
                transfers[j].push_back(&s_i_j.transfer(*parties[j]));
                if (verbose >= 3)
                    cout << parties[i]->name() << " sends to "
                         << parties[j]->name() << ": " << s_i_j.name() << endl;
            } else {
                transfers[j].push_back(&s_i_j);
            }
        }
    }
    if (verbose >= 3) cout << endl;

    Graph graph;
    std::vector<Share*> deltas;
    std::vector<Expression*> deltas_transferred;
    for (int i = 0; i < parties.size(); i++) {
        auto recv_queue = transfers[i];
        Share* delta_i = recv_queue.back();
        recv_queue.pop_back();
        while (recv_queue.size() > 0) {
            auto& partial_sum = *delta_i + *(recv_queue.back());
            recv_queue.pop_back();
            delta_i = &partial_sum;
        }
        deltas.push_back(delta_i);
    }
    for (int i = 0; i < parties.size(); i++) {
        deltas_transferred.clear();
        for (int j = 0; j < parties.size(); j++) {
            if (i == j)
                deltas_transferred.push_back(deltas[j]);
            else
                deltas_transferred.push_back(&deltas[j]->transfer(*parties[i]));
        }
        Share* subgraph_i =
            &Share::reconstruct(deltas_transferred, *parties[i]);
        if (verbose >= 3 && i == 0)
            cout << parties[i]->name() << " yield subgraph: " << *subgraph_i
                 << endl;
        graph.importFrontend(subgraph_i);
    }
    graph.initOutputNodes();
    graph.initSearchState();
    cout << "Initial graph has " << graph.nodeSize() << " nodes, "
         << graph.edgeSize() << " edges" << endl;
    cout << "Initial potential: " << Graph::to_string(graph.potential())
         << endl;
    if (verbose >= 3) cout << endl << "Initial graph:" << endl << graph << endl;
    bool proved = graph.tryProvingByPotential();
    cout << endl << "Proved? " << std::boolalpha << proved << endl;
    cout << "Final graph has " << graph.nodeSize() << " nodes, "
         << graph.edgeSize() << " edges" << endl;
    cout << "Final potential: " << Graph::to_string(graph.potential()) << endl;
    if (verbose >= 3) cout << "Final graph:" << endl << graph << endl;
    if (verbose >= 2) {
        cout << endl << "Transform history:" << endl;
        for (auto& entry : graph.transformTape) {
            cout << entry.node->getName() << " " << Graph::to_string(entry.type)
                 << " " << Graph::to_string(entry.potential) << endl;
        }
    }

    for (auto p : parties) delete p;
    for (auto s : secrets) delete s;
}

int main(int argc, char* argv[]) {
    if (argc <= 3) {
        cout << "Usage: " << argv[0] << " I T N" << endl;
        return 0;
    }
    auto I = atoi(argv[1]);
    auto T = atoi(argv[2]);
    auto N = atoi(argv[3]);
    int verbose;
    if (N <= 3)
        verbose = 3;
    else if (N <= 5)
        verbose = 2;
    else
        verbose = 1;
    test_bgw_add(I, T, N, verbose);
    return 0;
}
