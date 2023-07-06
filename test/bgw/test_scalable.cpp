#include <assert.h>

#include <cstdlib>
#include <iostream>
#include <memory>

#include "../../src/backend/builtin.hpp"
#include "../../src/bgwfrontend/builtin.hpp"
#include "prove_helper.hpp"

using namespace mpc;
using namespace std;

enum class BgwOp : int {
    ADD = 0,
    MUL = 1,
};

void rand_op(bgw::Variable& out, const bgw::Variable& var) {
    BgwOp op = static_cast<BgwOp>(rand() % 2);
    switch (op) {
    case BgwOp::ADD:
        out = out + var;
        break;
    case BgwOp::MUL:
        out = out * var;
        break;
    default:
        throw runtime_error(
            string("Bad op: ") + to_string(static_cast<int>(op)));
    }
}

void test_bgw_scalable(size_t I, size_t T, size_t N, size_t M, int verbose = 1) {
    Context& ctx = Context::get_context();

    auto c = Constant(ctx, "const");

    std::vector<PartyDecl*> parties;
    std::vector<Secret*> secrets;
    for (int i = 0; i < N; i++) {
        auto party = new PartyDecl(ctx, "p_" + to_string(i));
        parties.push_back(party);
    }

    for (int j = 0; j < M; j++) {
        for (int i = 0; i < N; i++) {
            auto secret = new Secret(ctx, "x_" + to_string(i) + "_" + to_string(j), *(parties[i]));
            secrets.push_back(secret);
        }
    }

    for (int i = 0; i < I; i++) {
        parties[i]->set_corrupted();
    }

    bgw::Context bgw_ctx(parties, T);

    // protocol described here
    auto protocol = make_shared<bgw::Variable>(bgw_ctx);
    auto var = make_shared<bgw::Variable>(bgw_ctx);

    *var = *secrets[0];
    *var = c * *var;
    *protocol = *var;
    for (int i = 1; i < M * N; i++) {
        *var = *secrets[i];
        rand_op(*protocol, *var);
    }

    auto graph = make_shared<Graph>();
    for (int i = 0; i < N; i++) {
        graph->importFrontend(&(protocol->yield(*parties[i])));
    }

    prove_by_hint(*graph, verbose);
}

// try: .\test_scalable 10 10 100 1
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
    test_bgw_scalable(I, T, N, M, verbose);
    return 0;
}
