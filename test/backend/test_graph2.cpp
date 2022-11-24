#include <assert.h>

#include <iostream>

#include "../../src/backend/builtin.hpp"
#include "../../src/mpcgraph/builtin.hpp"

using namespace mpc;
using namespace std;

Constant build_constant(Context& ctx, string val) {
    std::stringstream cs;
    cs << "_const_" << val;
    return Constant(ctx, cs.str());
}

/*
    p0 yield subgraph:
    <share[p0] share_56=RECONSTRUCT(
        <share[p0] share_39=ADD(
            <share[p0] share_37=ADD(
                <share[p0] share_36=ADD(
                    <share[p0] share_33=SCALARMUL(
                        <share[p0] share_16=EVAL(<poly{1}[p0][share_15] poly_3>)>,
                        <const _const_lambda_1>)>,
                    <share[p0] share_34=SCALARMUL(
                        <share[p0] share_23=TRANSFER(<share[p1] share_22=EVAL(<poly{1}[p1][share_21] poly_4>)>)>,
                        <const _const_lambda_2>)>)>,
                <share[p0] share_35=SCALARMUL(
                    <share[p0] share_29=TRANSFER(<share[p2] share_28=EVAL(<poly{1}[p2][share_27] poly_5>)>)>,
                    <const _const_lambda_3>)>)>,
            <share[p0] share_38=SCALARMUL(
                <share[p0] share_11=TRANSFER(<share[p2] share_10=EVAL(<poly{1}[p2][x2] poly_2>)>)>,
                <const _const_2>)>)>,
        <share[p0] share_47=TRANSFER(
            <share[p1] share_46=ADD(
                <share[p1] share_44=ADD(
                    <share[p1] share_43=ADD(
                        <share[p1] share_40=SCALARMUL(
                            <share[p1] share_18=TRANSFER(<share[p0] share_17=EVAL(<poly{1}[p0][share_15] poly_3>)>)>,
                            <const _const_lambda_1>)>,
                        <share[p1] share_41=SCALARMUL(
                            <share[p1] share_24=EVAL(<poly{1}[p1][share_21] poly_4>)>,
                            <const _const_lambda_2>)>)>,
                    <share[p1] share_42=SCALARMUL(
                        <share[p1] share_31=TRANSFER(<share[p2] share_30=EVAL(<poly{1}[p2][share_27] poly_5>)>)>,
                        <const _const_lambda_3>)>)>,
                <share[p1] share_45=SCALARMUL(
                    <share[p1] share_13=TRANSFER(<share[p2] share_12=EVAL(<poly{1}[p2][x2] poly_2>)>)>,
                    <const _const_2>)>)>)>,
        <share[p0] share_55=TRANSFER(
            <share[p2] share_54=ADD(
                <share[p2] share_52=ADD(
                    <share[p2] share_51=ADD(
                        <share[p2] share_48=SCALARMUL(
                            <share[p2] share_20=TRANSFER(<share[p0] share_19=EVAL(<poly{1}[p0][share_15] poly_3>)>)>,
                            <const _const_lambda_1>)>,
                        <share[p2] share_49=SCALARMUL(
                            <share[p2] share_26=TRANSFER(<share[p1] share_25=EVAL(<poly{1}[p1][share_21] poly_4>)>)>,
                            <const _const_lambda_2>)>)>,
                    <share[p2] share_50=SCALARMUL(
                        <share[p2] share_32=EVAL(<poly{1}[p2][share_27] poly_5>)>,
                        <const _const_lambda_3>)>)>,
                <share[p2] share_53=SCALARMUL(
                    <share[p2] share_14=EVAL(<poly{1}[p2][x2] poly_2>)>,
                    <const _const_2>)>)>)>)>
 */
void test_graph_bgw()
{
    Context& ctx = Context::get_context();

    constexpr const size_t SEC = 1;
    constexpr const size_t N = 2 * SEC + 1;

    std::vector<PartyDecl*> parties;
    std::vector<Secret*> secrets;
    for (int i = 0; i < N; i++) {
        auto party = new PartyDecl(ctx, "p" + to_string(i));
        auto secret = new Secret(ctx, "x" + to_string(i), *party);
        parties.push_back(party);
        secrets.push_back(secret);
    }
    for (int i = 0; i < SEC; i++) parties[i]->set_corrupted();

    std::vector<std::vector<Share*>> transfers;
    transfers.resize(parties.size());

    for (int i = 0; i < parties.size(); i++) {
        auto& q_i_x = Poly::gen_poly(ctx, *parties[i], *secrets[i], SEC);
        for (int j = 0; j < parties.size(); j++) {
            auto& s_i_j = q_i_x.eval(*parties[j]).transfer(*parties[j]);
            transfers[j].push_back(&s_i_j);
            cout << parties[i]->name() << " sends to " << parties[j]->name()
                 << ": " << s_i_j << endl;
        }
    }

    cout << endl;

    // x * y + 2 * z
    Graph graph;
    std::vector<Expression*> deltas;
    Constant c = build_constant(ctx, std::to_string(2));
    Constant lambda_1 = build_constant(ctx, "lambda_1");
    Constant lambda_2 = build_constant(ctx, "lambda_2");
    Constant lambda_3 = build_constant(ctx, "lambda_3");
    for (int i = 0; i < N; i++) {
        auto& partial_mul = *(transfers[i][0]) * *(transfers[i][1]);
        auto& q_i_x = Poly::gen_poly(ctx, *parties[i], partial_mul, SEC);
        for (int j = 0; j < N; j++) {
            auto& s_i_j = q_i_x.eval(*parties[j]).transfer(*parties[j]);
            transfers[j].push_back(&s_i_j);
            cout << parties[i]->name() << " sends to " << parties[j]->name()
                 << ": " << s_i_j << endl;
        }
    }
    for (int i = 0; i < N; i++) {
        Share* mul_delta_1 = &(transfers[i][3])->scalarMul(lambda_1);
        Share* mul_delta_2 = &(transfers[i][4])->scalarMul(lambda_2);
        Share* mul_delta_3 = &(transfers[i][5])->scalarMul(lambda_3);
        auto& partial_sum = *mul_delta_1 + *mul_delta_2 + *mul_delta_3;
        Share* delta_i = &(partial_sum + (transfers[i][2])->scalarMul(c));
        deltas.push_back(&(delta_i->transfer(*parties[0])));
    }
    for (int i = 0; i < N; i++) {
        Share* subgraph_i = &Share::reconstruct(deltas, *parties[i]);
        cout << parties[i]->name() << " yield subgraph: " << *subgraph_i
             << endl;
        graph.importFrontend(subgraph_i);
    }
    graph.initOutputNodes();
    graph.initSearchState();
    cout << endl << "Init graph:" << endl << graph << endl;

    GraphVec histories;
    bool proved = graph.tryProving(histories);
    cout << endl << "Proved? " << std::boolalpha << proved << endl;
    cout << "Result graph:" << endl << graph << endl;
    cout << endl << "Transform history:" << endl;
    for (auto& pair : histories.back()->transformTape) {
        cout << pair.first->getName() << " " << Graph::to_string(pair.second)
             << endl;
    }

    for (auto p : parties) delete p;
    for (auto s : secrets) delete s;
}

int main()
{
    test_graph_bgw();
    return 0;
}
