#include <assert.h>

#include <iostream>

#include "../../src/backend/builtin.hpp"
#include "../../src/mpcgraph/builtin.hpp"
#include "../../src/bgwfrontend/builtin.hpp"
#include "../bgw/prove_helper.hpp"

using namespace mpc;
using namespace std;

int I, T, N;

int main(int argc, char* argv[]) {
    // parse command-line arguments
    if (argc < 4) {
        cout << "Usage: " << " I(the number of corrupted parties) T(the threshold) N(the number of parties)" << endl;
        return 0;
    }
    I = atoi(argv[1]);
    T = atoi(argv[2]);
    N = atoi(argv[3]);

    Context& ctx = Context::get_context();

    vector<PartyDecl*> parties;
    for (int i = 0; i < N; ++i) {
        parties.push_back(new PartyDecl(ctx, "p" + to_string(i)));
    }
    for (int i = 0; i < I; ++i) parties[i]->set_corrupted();

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
    auto &masked_bin(masked_bin_sharing.yield(parties[0]));
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
    std::vector<Share *> outputs;
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
        outputs.push_back(&output_sharing_i.yield(parties[i]));
    }
    
    auto graph = make_shared<Graph>();
    for (auto output: outputs)
        graph->importFrontend(output);
    prove_by_potential(*graph);
}
