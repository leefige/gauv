#include <cassert>

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <iostream>
#include <vector>
#include <unordered_set>

#include "../../src/backend/builtin.hpp"
#include "../../src/bgwfrontend/builtin.hpp"

using namespace mpc;
using namespace std;

void test_bgw_add(size_t I, size_t T, size_t N, size_t M) {
    // calculate sum of secrets
    Context& ctx = Context::get_context();

    std::vector<PartyDecl*> parties;
    std::vector<Expression*> secrets;
    for (unsigned i = 0; i < N; i++) {
        auto party = new PartyDecl(ctx, "p" + to_string(i));
        parties.push_back(party);
        for (unsigned j = 0; j < M; j++) {
            auto secret = new Secret(ctx, "x" + to_string(i * M + j), ArithFieldType::get_arith_field_type(), party);
            secrets.push_back(secret);
        }
    }

    bgw::Context bgw_ctx(parties, T);
    std::vector<bgw::Variable> x;
    // protocol described here
    bgw::Variable protocol(bgw_ctx, *secrets[0]);
    for (unsigned i = 1; i < N * M; i++) {
        bgw::Variable var(bgw_ctx, *secrets[i]);
        protocol = protocol + var;
    }

    vector<Expression*> outputs;
    for (unsigned i = 0; i < N; i++)
        outputs.push_back(&protocol.yield(parties[i]));

    GraphBaseBuilder builder(outputs);
    GraphBase graph_base = builder.build();
    Prover prover(graph_base,
        vector<unordered_set<PartyDecl*>>{
            unordered_set<PartyDecl*>(parties.begin(), parties.end())
        },
        parties,
        T);
    prover.prove(I);
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

    // change log pattern
    spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
#ifdef DEBUG // if in the debug version
    spdlog::set_level(spdlog::level::trace); // default level is "info"
#endif

    test_bgw_add(I, T, N, M);
    return 0;
}
