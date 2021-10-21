#include "../../src/mpclib/builtin.hpp"
#include "../../src/mpclib/defs.hpp"
#include "../../src/mpclib/context.hpp"

#include <iostream>

#include <assert.h>

using namespace mpc;

constexpr base_t BASE = 23U;

constexpr sec_t SEC = 1;

using F = field<BASE>;
using F_other = field<23>;

int main()
{
    F val0(45);
    F val1(9);
    F val2(18);

    par p0(0, F(23));
    par p1(1, F(5));
    par p2(2, F(17));

    std::cout << p0 << std::endl;
    std::cout << p1 << std::endl;
    std::cout << p2 << std::endl;

    parset<BASE> ps{{p0, p1, p2}};

    auto q_0_x = poly_gen<BASE, SEC>(p0, val0);
    auto q_1_x = poly_gen<BASE, SEC>(p1, val1);
    auto q_2_x = poly_gen<BASE, SEC>(p2, val2);

    std::cout << q_0_x << std::endl;
    std::cout << q_1_x << std::endl;
    std::cout << q_2_x << std::endl;

    std::cout << q_0_x.eval(p0) << " " << q_0_x.eval(p1) << " " << q_0_x.eval(p2) << std::endl;


    std::shared_ptr<mpc_context<BASE>> ctx = mpc_context<BASE>::get_context();
    ctx->register_party(p0);
    ctx->register_party(p1);

    share<BASE> s_0_1 = q_0_x.eval(p1);
    std::cout << s_0_1 << std::endl;

    ctx->send(p0, p1, s_0_1);
    auto rcved = ctx->receive(p1);
    std::cout << rcved.value() << std::endl;

    return 0;
}
