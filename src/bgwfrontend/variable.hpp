#pragma once

#include "common.hpp"
#include "context.hpp"

namespace bgw {
class Variable {
    const Context& _ctx;
    ShareVec _shares;

   public:
    Variable(const Context& ctx) : _ctx(ctx) {}

    const Context& context() const { return _ctx; }
    const ShareVec& shares() const { return _shares; }

    Variable& operator=(mpc::Secret& sec) {
        auto& poly =
            mpc::Poly::gen_poly(sec.context(), sec.party(), sec, _ctx.T());
        for (auto p : _ctx.parties()) {
            auto& s = poly.eval(*p);
            if (p == &sec.party())
                _shares.push_back(&s);
            else
                _shares.push_back(&s.transfer(*p));
        }
        return *this;
    }
    Variable& operator=(mpc::Share& sec) {
        auto& poly =
            mpc::Poly::gen_poly(sec.context(), sec.party(), sec, _ctx.T());
        for (auto p : _ctx.parties()) {
            auto& s = poly.eval(*p);
            if (p == &sec.party())
                _shares.push_back(&s);
            else
                _shares.push_back(&s.transfer(*p));
        }
        return *this;
    }

    mpc::Share& yield(const mpc::PartyDecl& party) const {
        std::vector<mpc::Expression*> transferred;
        for (auto s : _shares) {
            if (&s->party() == &party) {
                transferred.push_back(s);
            } else {
                transferred.push_back(&s->transfer(party));
            }
        }
        return mpc::Share::reconstruct(transferred, party);
    }

    friend Variable& operator+(const Variable& lhs, const Variable& rhs) {
        auto& ret = *new Variable(lhs._ctx);
        auto N = lhs._ctx.N();
        for (int i = 0; i < N; i++)
            ret._shares.push_back(&(*lhs._shares[i] + *rhs._shares[i]));
        return ret;
    }

    friend Variable& operator*(const Variable& lhs, mpc::Constant& c) {
        auto& ret = *new Variable(lhs._ctx);
        auto N = lhs._ctx.N();
        for (int i = 0; i < N; i++)
            ret._shares.push_back(&(*lhs._shares[i] * c));
        return ret;
    }
    friend Variable& operator*(mpc::Constant& c, const Variable& rhs) {
        return rhs * c;
    }

    friend Variable& operator*(const Variable& lhs, const Variable& rhs) {
        static size_t mulCounter = 0;
        auto& ctx = lhs._ctx;
        auto N = ctx.N();
        Variable* sum;
        for (int i = 0; i < N; i++) {
            Variable inter(ctx);
            inter = *lhs._shares[i] * *rhs._shares[i];
            auto& lambda =
                *new mpc::Constant(lhs._shares.front()->context(),
                                   "mul_" + std::to_string(mulCounter) +
                                       "_lambda_" + std::to_string(i));
            auto& x = inter * lambda;
            if (i == 0)
                sum = &x;
            else
                sum = &(*sum + x);
        }
        mulCounter++;
        return *sum;
    }
};
}  // namespace bgw
