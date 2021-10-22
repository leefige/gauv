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

    par p0(0, F(23));
    par p1(1, F(5));

    std::cout << p0 << std::endl;
    std::cout << p1 << std::endl;

    parset<BASE> ps{p0, p1};

    auto q_0_x = poly_gen<BASE, SEC>(p0, val0);

    auto ctx = mpc_context<BASE>::get_context();
    ctx->register_party(p0);
    ctx->register_party(p1);

    share<BASE> s_0_1 = q_0_x.eval(p1);
    std::cout << s_0_1 << std::endl;

    ctx->send(p0, p1, s_0_1);
    auto rcved = ctx->receive(p1);
    std::cout << rcved.value() << std::endl;

    p0.send(p1, s_0_1);
    auto sigma_0_1 = p1.receive();
    std::cout << sigma_0_1 << std::endl;
    assert(s_0_1 == sigma_0_1);

    return 0;
}
