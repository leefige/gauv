#include "../../src/mpclib/mpc.hpp"

#include <iostream>

#include <assert.h>

using namespace mpc;

constexpr base_t BASE = 97U;

using F = field<BASE>;
using F_other = field<23>;

int main() {
    F val1(5);
    F val2(17);

    F_other bad(11);

    assert(val2 != val1);
    // static_assert(bad != val1);

    par p0(0, val1);
    par p1(1, val2);
    assert(p0 != p1);

    std::cout << p0 << std::endl;
    std::cout << p1 << std::endl;

    parset ps{p0, p1};

    parset pp{p0};
    parset ps2 = {p0};
    assert(pp[0] == ps[0]);
    assert(pp[0] == ps2[0]);

    std::cout << ps2[0] << " " << ps[1] << std::endl;

    // 8 + x + 3 * x^2
    poly<BASE, 2> qx(p1, F(8), {F(1), F(3)});
    std::cout << qx.eval(p0) << std::endl;

    return 0;
}
