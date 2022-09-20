#include <assert.h>

#include <iostream>

#include "../../src/backend/builtin.hpp"
#include "../../src/mpcgraph/builtin.hpp"

using namespace mpc;
using namespace std;

/*
    p0 yield subgraph: 
    <share[p0] share_26=RECONSTRUCT(
        <share[p0] share_17=ADD(
            <share[p0] share_15=MUL(
                <share[p0] share_0=EVAL(<poly{1}[p0][x0] poly_0>)>,
                <share[p0] share_6=TRANSFER(
                    <share[p1] share_5=EVAL(
                        <poly{1}[p1][x1] poly_1>)>)>)>,
            <share[p0] share_16=SCALARMUL(
                <share[p0] share_11=TRANSFER(
                    <share[p2] share_10=EVAL(
                        <poly{1}[p2][x2] poly_2>)>)>,
                <const _const_2>)>)>,
    <share[p0] share_21=TRANSFER(
        <share[p1] share_20=ADD(
            <share[p1] share_18=MUL(
                <share[p1] share_2=TRANSFER(
                    <share[p0] share_1=EVAL(
                        <poly{1}[p0][x0] poly_0>)>)>,
                <share[p1] share_7=EVAL(
                    <poly{1}[p1][x1] poly_1>)>)>,
            <share[p1] share_19=SCALARMUL(
                <share[p1] share_13=TRANSFER(
                    <share[p2] share_12=EVAL(
                        <poly{1}[p2][x2] poly_2>)>)>,
                <const _const_2>)>)>)>,
    <share[p0] share_25=TRANSFER(
        <share[p2] share_24=ADD(
            <share[p2] share_22=MUL(
                <share[p2] share_4=TRANSFER(
                    <share[p0] share_3=EVAL(
                        <poly{1}[p0][x0] poly_0>)>)>,
                <share[p2] share_9=TRANSFER(
                    <share[p1] share_8=EVAL(
                        <poly{1}[p1][x1] poly_1>)>)>)>,
            <share[p2] share_23=SCALARMUL(
                <share[p2] share_14=EVAL(
                    <poly{1}[p2][x2] poly_2>)>,
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

    Graph graph;
    std::vector<Expression*> deltas;
    std::stringstream cs;
    cs << "_const_" << std::to_string(2);
    Constant c = Constant(ctx, cs.str());
    for (int i = 0; i < N; i++) {
        Share* delta_i = &(*(transfers[i][0]) * *(transfers[i][1]) + (transfers[i][2])->scalarMul(c));
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
    bool proved = graph.tryProving();
    cout << endl << "Proved? " << std::boolalpha << proved << endl;
    cout << "Result graph:" << endl << graph << endl;
    cout << endl << "Transform history:" << endl;
    for (auto& pair : graph.transformTape) {
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