#include <cassert>

#include <spdlog/spdlog.h>

#include <iostream>

#include "../../src/backend/builtin.hpp"
#include "../../src/mpcgraph/builtin.hpp"
#include "../../src/bgwfrontend/builtin.hpp"

using namespace mpc;
using namespace std;

int I, T, N;

int main(int argc, char* argv[]) {
    // parse command-line arguments
    if (argc < 4) {
        cout << "Usage: " << " I(the number of corrupted parties) T(the threshold) N(the number of parties)" << endl;
        return 0;
    }
    I = atoi(argv[1]); // 这里我们最好也还支持可变的 I
    T = atoi(argv[2]);
    N = atoi(argv[3]);
    
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%^%l%$] %v");
#ifdef DEBUG // if in the debug version
    spdlog::set_level(spdlog::level::trace); // default level is "info"
#endif

    Context& ctx = Context::get_context();

    vector<PartyDecl*> parties;
    for (int i = 0; i < N; ++i) {
        parties.push_back(new PartyDecl(ctx, "p" + to_string(i)));
    }

    auto c2 = Constant(ctx, "const_-2");

    bgw::Context bgw_ctx(parties, T);

    // inputs
    bgw::Variable b(bgw_ctx, ctx, BinFieldType::get_bin_field_type());
    std::vector<mpc::Share *> betas;
    for (int i = 0; i < T; ++i)
        betas.push_back(&mpc::Share::gen_share(
            ctx, ArithFieldType::get_arith_field_type(), parties[i]));

    // protocol described here
    std::vector<bgw::Variable *> local_r_bin;
    std::vector<bgw::Variable *> local_r_arith;
    for (int i = 0; i < N; ++i) {
        Randomness* r_i_bin = new Randomness(ctx,
            "r_" + std::to_string(i) + "_bin",
            BinFieldType::get_bin_field_type(),
            parties[i]);
        local_r_bin.push_back(new bgw::Variable(bgw_ctx, *r_i_bin));

        TypeCast* r_i_arith = new TypeCast(ctx,
            "r_" + std::to_string(i) + "_arith",
            ArithFieldType::get_arith_field_type(),
            r_i_bin,
            parties[i]);
        local_r_arith.push_back(new bgw::Variable(bgw_ctx, *r_i_arith));
    }
    bgw::Variable global_r_bin(*local_r_bin[0]);
    for (int i = 1; i < N; ++i)
        global_r_bin = global_r_bin + *local_r_bin[i];
    bgw::Variable global_r_arith(*local_r_arith[0]);
    for (int i = 1; i < N; ++i)
        global_r_arith = global_r_arith + *local_r_arith[i] + global_r_arith * *local_r_arith[i] * c2;
    auto masked_bin_sharing = b + global_r_bin;

    // reconstruct
    auto &masked_bin(masked_bin_sharing.yield(parties[0], "b_xor_r_bin")); // 2023-07-21 19:44:00 感觉这个图应该还没建对，在 1 2 3 时，reconstruction 只有 3 个，但应该是 2 个
    TypeCast* masked_arith = new TypeCast(ctx,
        "b_xor_r_arith",
        ArithFieldType::get_arith_field_type(),
        &masked_bin,
        parties[0]
    );
    bgw::Variable masked_arith_sharing(bgw_ctx, *masked_arith);
    bgw::Variable init_arith_sharing = masked_arith_sharing + global_r_arith + masked_arith_sharing * global_r_arith * c2;

    // reshare to align the first T shares as provided
    std::vector<Share *> init_arith_shares(init_arith_sharing.shares());
    std::vector<bgw::Variable *> reshared_arith_sharings;
    for (int i = 0; i < N; ++i) {
        reshared_arith_sharings.push_back(new bgw::Variable(bgw_ctx, *init_arith_shares[i]));
    }
    std::vector<bgw::Variable *> reshared_beta_sharings;
    for (int i = 0; i < T; ++i) {
        reshared_beta_sharings.push_back(new bgw::Variable(bgw_ctx, *betas[i]));
    }
    std::vector<Expression *> outputs;
    for (int i = T; i < N; ++i) {
        bgw::Variable output_sharing_i(bgw_ctx);
        for (int j = 0; j < T; ++j) {
            auto &l = *new mpc::Constant(ctx, "l^" + std::to_string(i) + "_" + std::to_string(j));
            if (j == 0)
                output_sharing_i = *reshared_beta_sharings[j] * l;
            else
                output_sharing_i = output_sharing_i + *reshared_beta_sharings[j] * l;
        }
        for (int j = 0; j < N; ++j) {
            auto &l = *new mpc::Constant(ctx, "l_0^" + std::to_string(i) + "*l^0_" + std::to_string(j));
            output_sharing_i = output_sharing_i + *reshared_arith_sharings[j] * l;
        }
        outputs.push_back(&output_sharing_i.yield(parties[i], "beta_" + std::to_string(i)));
    }

    // 把 party 分成若干个等价类
    std::unordered_set<PartyDecl *> class0({ parties[0] });
    std::unordered_set<PartyDecl *> class1;
    for (int i = 1; i < T; ++i) class1.insert(parties[i]);
    std::unordered_set<PartyDecl *> class2;
    for (int i = T; i < N; ++i) class2.insert(parties[i]);
    std::vector<std::unordered_set<PartyDecl *>> equivalent_classes({class0, class1, class2});

    GraphBaseBuilder builder(outputs);
    GraphBase graph_base = builder.build();
    Prover prover(graph_base, equivalent_classes, parties, T);
    prover.prove(I);
}
