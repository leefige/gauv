#include <cassert>

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <iostream>
#include <memory>

#include "../../src/backend/builtin.hpp"
#include "../../src/bgwfrontend/builtin.hpp"

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

void test_bgw_scalable(size_t I, size_t T, size_t N, size_t M) {
    Context& ctx = Context::get_context();

    auto c = Constant(ctx, "const");

    std::vector<PartyDecl*> parties;
    std::vector<Secret*> secrets;
    for (size_t i = 0; i < N; i++) {
        parties.push_back(new PartyDecl(ctx, "p_" + to_string(i)));
    }

    for (size_t j = 0; j < M; j++) {
        for (size_t i = 0; i < N; i++) {
            auto secret = new Secret(ctx, "x_" + to_string(i) + "_" + to_string(j), ArithFieldType::get_arith_field_type(), parties[i]);
            secrets.push_back(secret);
        }
    }

    bgw::Context bgw_ctx(parties, T);

    // protocol described here
    auto protocol = make_shared<bgw::Variable>(bgw_ctx);
    auto var = make_shared<bgw::Variable>(bgw_ctx);

    *var = *secrets[0];
    *var = c * *var;
    *protocol = *var;
    for (size_t i = 1; i < M * N; i++) {
        *var = *secrets[i];
        rand_op(*protocol, *var);
    }

    vector<Expression*> outputs;
    for (size_t i = 0; i < N; i++)
        outputs.push_back(&protocol->yield(parties[i]));

    GraphBaseBuilder builder(outputs);
    GraphBase graph_base = builder.build();
    Prover prover(graph_base,
        vector<unordered_set<PartyDecl*>>{
            unordered_set<PartyDecl*>(parties.begin(), parties.end())
        },
        parties,
        T);
    if (I == 0) prover.prove();
    else prover.prove(I);
}

// try: .\test_scalable 10 10 100 1
int main(int argc, char* argv[]) {
    if (argc <= 4) {
        cout << "Usage: " << argv[0] << " I(the number of corrupted parties) T(the threshold) N(the number of parties) M(how many inputs does each party have)" << endl;
        return 0;
    }
    size_t I;
    if (strcmp(argv[1], "I") == 0) {
        // 在参数里，我们用“I“来支持表示可变的 I，在程序里我们用 0 来表示
        I = 0;
    }
    else I = atoi(argv[1]);
    auto T = atoi(argv[2]);
    auto N = atoi(argv[3]);
    auto M = atoi(argv[4]);

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%^%l%$] %v");
#ifdef DEBUG // if in the debug version
    spdlog::set_level(spdlog::level::trace); // default level is "info"
#endif

    test_bgw_scalable(I, T, N, M);
    return 0;
}
