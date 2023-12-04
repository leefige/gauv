#include <cassert>

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <iostream>

#include "../../src/backend/builtin.hpp"
#include "../../src/bgwfrontend/builtin.hpp"

using namespace mpc;
using namespace std;

void test_full_bgw() {
    // x * y + 2 * z
    constexpr size_t T = 1;
    constexpr size_t N = 3;
    Context& ctx = Context::get_context();

    std::vector<PartyDecl*> parties;
    std::vector<Secret*> secrets;
    for (size_t i = 0; i < N; i++) {
        auto party = new PartyDecl(ctx, "p" + to_string(i));
        auto secret = new Secret(ctx, "x" + to_string(i), ArithFieldType::get_arith_field_type(), party);
        parties.push_back(party);
        secrets.push_back(secret);
    }

    bgw::Context bgw_ctx(parties, T);
    std::vector<bgw::Variable> x;
    for (size_t i = 0; i < N; i++)
        x.push_back(bgw::Variable(bgw_ctx, *secrets[i]));
    // protocol described here
    auto c = Constant(ctx, "const_2");
    auto protocol = x[0] * x[1] + c * x[2];

    vector<Expression*> outputs;
    for (size_t i = 0; i < N; i++)
        outputs.push_back(&protocol.yield(parties[i]));

    GraphBaseBuilder builder(outputs);
    GraphBase graph_base = builder.build();
    Prover prover(graph_base,
        vector<unordered_set<PartyDecl*>>{
            unordered_set<PartyDecl*>(parties.begin(), parties.end())
        },
        parties,
        T);
}

int main(int argc, char* argv[]) {
    if (argc != 1) {
        cout << "Usage: " << argv[0] << endl;
        return 0;
    }

    // change log pattern
    spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
#ifdef DEBUG // if in the debug version
    spdlog::set_level(spdlog::level::trace); // default level is "info"
#endif

    test_full_bgw();
    return 0;
}
