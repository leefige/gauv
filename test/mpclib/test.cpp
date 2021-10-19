#include "../../src/mpclib/mpc.hpp"

#include <iostream>

#include <assert.h>

using namespace mpc;

constexpr base_t BASE = 23U;

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

    // q(x) = 8 + x + 3 * x^2
    poly<BASE, 2> qx(p1, F(8), {F(1), F(3)});
    // q(5) = 88
    std::cout << qx.eval(p0) << std::endl;


    share<BASE> s11(p1, F(6));
    share<BASE> s12(p1, F(7));
    share<BASE> s01(p0, F(11));

    try {
        s11 + s01;
    } catch (const std::exception& e) {
        std::cout << "Caught! " << e.what() << std::endl;
    }

    auto s13 = s11 + s12;
    auto s14 = s11 * s12;
    std::cout << s13 << " " << s14 << std::endl;


    bundle<BASE> b0(ps, {s01, s11});

    try {
        bundle<BASE> b1(ps, {s12, s11});
    } catch (const std::exception& e) {
        std::cout << "Caught! " << e.what() << std::endl;
    }

    try {
        bundle<BASE> b2(ps, {s12});
    } catch (const std::exception& e) {
        std::cout << "Caught! " << e.what() << std::endl;
    }

    std::cout << b0[p0] << " " << b0[p1] << std::endl;

    share<BASE> s02(p0, F(15));
    bundle<BASE> b1(ps, {s02, s13});
    std::cout << b1[p0] << " " << b1[p1] << std::endl;

    try {
        bundle<BASE> b2(pp, {s01});
        auto b3 = b0 + b2;
    } catch (const std::exception& e) {
        std::cout << "Caught! " << e.what() << std::endl;
    }

    auto b3 = b0 + b1;
    std::cout << b3[p0] << b3[p1] << std::endl;

    auto b4 = b0 * b1;
    std::cout << b4[p0] << b4[p1] << std::endl;
    return 0;
}
