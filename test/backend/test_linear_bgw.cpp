#include <assert.h>

#include <cstdlib>
#include <iostream>

#include "../../src/backend/builtin.hpp"
#include "../../src/mpcgraph/builtin.hpp"

using namespace mpc;
using namespace std;

void test_linear_bgw(size_t I, size_t T) {
    // a + b + 2 * c
    constexpr size_t N = 3;
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

    // input sharing
    for (int i = 0; i < parties.size(); i++) {
        auto& q_i_x = Poly::gen_poly(ctx, *parties[i], *secrets[i], T);
        for (int j = 0; j < parties.size(); j++) {
            auto& s_i_j = q_i_x.eval(*parties[j]);
            if (i != j) {
                transfers[j].push_back(&s_i_j.transfer(*parties[j]));
                cout << parties[i]->name() << " sends to " << parties[j]->name()
                     << ": " << s_i_j.name() << endl;
            } else {
                transfers[j].push_back(&s_i_j);
            }
        }
    }
    cout << endl;

    Graph graph;
    std::vector<Share*> deltas;
    std::vector<Expression*> deltas_transferred;
    auto c = Constant::build_constant(ctx, "2");
    for (int i = 0; i < parties.size(); i++) {
        auto recv_queue = transfers[i];
        deltas.push_back(
            &(*recv_queue[0] + *recv_queue[1] + recv_queue[2]->scalarMul(c)));
    }
    // output sharing
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
        if (i == 0)
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
    cout << endl << "Initial graph:" << endl << graph << endl;
    bool proved = graph.tryProvingByPotential();
    cout << endl << "Proved? " << std::boolalpha << proved << endl;
    cout << "Final graph has " << graph.nodeSize() << " nodes, "
         << graph.edgeSize() << " edges" << endl;
    cout << "Final potential: " << Graph::to_string(graph.potential()) << endl;
    cout << "Final graph:" << endl << graph << endl;
    cout << endl << "Transform history:" << endl;
    for (auto& entry : graph.transformTape) {
        cout << entry.node->getName() << " " << Graph::to_string(entry.type)
             << " " << Graph::to_string(entry.potential) << endl;
    }

    for (auto p : parties) delete p;
    for (auto s : secrets) delete s;
}

int main(int argc, char* argv[]) {
    if (argc != 1 && argc != 3) {
        cout << "Usage: " << argv[0] << " I T" << endl;
        return 0;
    }
    int I = 1, T = 1;
    if (argc == 3) {
        I = atoi(argv[1]);
        T = atoi(argv[2]);
    }
    test_linear_bgw(I, T);
    return 0;
}
