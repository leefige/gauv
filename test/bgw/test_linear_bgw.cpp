#include <assert.h>

#include <cstdlib>
#include <iostream>

#include "../../src/backend/builtin.hpp"
#include "../../src/bgwfrontend/builtin.hpp"
#include "prove_helper.hpp"

using namespace mpc;
using namespace std;

void test_linear_bgw(size_t I, size_t T) {
    // x + y + 2 * z
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

    bgw::Context bgw_ctx(parties, T);
    std::vector<bgw::Variable> x;
    for (int i = 0; i < N; i++)
        x.push_back(bgw::Variable(bgw_ctx) = *secrets[i]);
    // protocol described here
    auto c = Constant(ctx, "const_2");
    auto protocol = x[0] + x[1] + c * x[2];

    Graph graph;
    for (int i = 0; i < N; i++)
        graph.importFrontend(&protocol.yield(*parties[i]));

    prove_by_potential(graph);
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
