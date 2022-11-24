#include <assert.h>

#include <cstdlib>
#include <iostream>

#include "../../src/backend/builtin.hpp"
#include "../../src/bgwfrontend/builtin.hpp"
#include "prove_helper.hpp"

using namespace mpc;
using namespace std;

void test_bgw_add(size_t I, size_t T, size_t N, int verbose = 1) {
    // calculate sum of secrets
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
    // protocol described here
    auto protocol = &(*new bgw::Variable(bgw_ctx) = *secrets[0]);
    for (int i = 1; i < N; i++)
        protocol = &(*protocol + (bgw::Variable(bgw_ctx) = *secrets[i]));

    Graph graph;
    for (int i = 0; i < N; i++)
        graph.importFrontend(&protocol->yield(*parties[i]));

    prove_helper(graph, verbose);
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
