#include "../../src/mpclib/defs.hpp"
#include "../../src/mpclib/builtin.hpp"
#include "../../src/mpclib/context.hpp"
#include "../../src/mpclib/protocol.hpp"

#include <iostream>

#include <assert.h>

using namespace mpc;

constexpr base_t BASE = 23U;
constexpr sec_t SEC = 1;

using F = field<BASE>;

auto ctx = mpc_context<BASE>::get_context();

int main() {
    F val0(45);
    F val1(9);
    F val2(18);

    par p0(0, F(23));
    par p1(1, F(5));
    par p2(2, F(17));

    std::cout << p0 << std::endl;
    std::cout << p1 << std::endl;
    std::cout << p2 << std::endl;

    parset<BASE> ps{p0, p1, p2};

    ctx->register_parties(ps);


    auto q_0_x = poly_gen<BASE, SEC>(p0, val0);
    auto q_1_x = poly_gen<BASE, SEC>(p1, val1);
    auto q_2_x = poly_gen<BASE, SEC>(p2, val2);
    bundle<BASE> shares_0(ps, {q_0_x.eval(p0), q_0_x.eval(p1), q_0_x.eval(p2)});

    protocol<BASE, SEC> f_rand_2t(ps, [&ps](){
        for (const par<BASE>& i : ps) {
            auto q_i_x = poly_gen<BASE, SEC * 2>(i, F(0));

            for (const par<BASE>& j : ps) {
                auto s_i_j = q_i_x.eval(j);
                i.send(j, s_i_j);
                std::cout << i.idx() << " send to " << j.idx() << ": " << s_i_j << std::endl;
            }
        }

        // sync()?
        std::vector<share<BASE>> res;
        for (const par<BASE>& i : ps) {
            share<BASE> delta_i(i);
            for (const par<BASE>& j : ps) {
                share<BASE> s_j_i = i.receive(j);
                std::cout << i.idx() << " receive from " << j.idx() << ": " << s_j_i << std::endl;
                delta_i = delta_i + s_j_i;
            }

            // each party outputs delta_i
            res.push_back(delta_i);
        }
        proto_out(bundle<BASE>(ps, res));
    });

    auto res = f_rand_2t();

    return 0;
}
