#include <assert.h>

#include <cstdlib>
#include <iostream>

#include "../../src/backend/builtin.hpp"
#include "../../src/bgwfrontend/builtin.hpp"
#include "prove_helper.hpp"

using namespace mpc;
using namespace std;

void test_bgw_add(size_t I, size_t T, size_t N, size_t M, int verbose = 1) {
    // calculate sum of secrets
    Context& ctx = Context::get_context();

    std::vector<PartyDecl*> parties;
    std::vector<Secret*> secrets;
    for (int i = 0; i < N; i++) {
        auto party = new PartyDecl(ctx, "p" + to_string(i));
        parties.push_back(party);
        for (int j = 0; j < M; j++) {
            auto secret = new Secret(ctx, "x" + to_string(i * M + j), ArithFieldType::get_arith_field_type(), party);
            secrets.push_back(secret);
        }
    }
    for (int i = 0; i < I; i++) parties[i]->set_corrupted();

    bgw::Context bgw_ctx(parties, T);
    // protocol described here
    bgw::Variable protocol(bgw_ctx), var(bgw_ctx);
    protocol = *secrets[0];
    for (int i = 1; i < N * M; i++) {
        var = *secrets[i];
        protocol = protocol * var;
    }

    Graph graph;
    for (int i = 0; i < N; i++)
        graph.importFrontend(&protocol.yield(parties[i]));

    prove_by_hint(graph, verbose);
}

int main(int argc, char* argv[]) {
    if (argc <= 4) {
        cout << "Usage: " << argv[0] << " I T N M" << endl;
        return 0;
    }
    auto I = atoi(argv[1]);
    auto T = atoi(argv[2]);
    auto N = atoi(argv[3]);
    auto M = atoi(argv[4]);
    int verbose;
    if (N * M <= 3)
        verbose = 3;
    else if (N * M <= 5)
        verbose = 2;
    else
        verbose = 1;
    test_bgw_add(I, T, N, M, verbose);
    return 0;
}
