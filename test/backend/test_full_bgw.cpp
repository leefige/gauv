#include <assert.h>

#include <cstdlib>
#include <iostream>

#include "../../src/backend/builtin.hpp"
#include "../../src/mpcgraph/builtin.hpp"

using namespace mpc;
using namespace std;

void test_full_bgw(int cor) {
    // a * b + 2 * c
    constexpr size_t T = 1;
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
    parties[cor]->set_corrupted();

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
    auto c = Constant::build_constant(ctx, std::to_string(2));
    auto lambda_1 = Constant::build_constant(ctx, "lambda_1");
    auto lambda_2 = Constant::build_constant(ctx, "lambda_2");
    auto lambda_3 = Constant::build_constant(ctx, "lambda_3");
    for (int i = 0; i < N; i++) {
        auto& partial_mul = *(transfers[i][0]) * *(transfers[i][1]);
        auto& q_i_x = Poly::gen_poly(ctx, *parties[i], partial_mul, T);
        for (int j = 0; j < N; j++) {
            auto& s_i_j = q_i_x.eval(*parties[j]);
            if (i == j) {
                transfers[j].push_back(&s_i_j);
            } else {
                transfers[j].push_back(&s_i_j.transfer(*parties[j]));
                cout << parties[i]->name() << " sends to " << parties[j]->name()
                     << ": " << s_i_j.name() << endl;
            }
        }
    }
    for (int i = 0; i < N; i++) {
        auto& mul_delta_1 = lambda_1 * *transfers[i][3];
        auto& mul_delta_2 = lambda_2 * *transfers[i][4];
        auto& mul_delta_3 = lambda_3 * *transfers[i][5];
        auto& partial_sum = mul_delta_1 + mul_delta_2 + mul_delta_3;
        auto& delta_i = partial_sum + c * *transfers[i][2];
        deltas.push_back(&delta_i);
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
    if (argc != 1 && argc != 2) {
        cout << "Usage: " << argv[0] << " corrupted_party" << endl;
        return 0;
    }
    int cor = 0;
    if (argc == 2) {
        cor = atoi(argv[1]);
    }
    test_full_bgw(cor);
    return 0;
}
