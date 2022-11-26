#pragma once

#include "common.hpp"
#include "context.hpp"

namespace bgw {
class Variable {
    const Context& _ctx;
    ShareVec _shares;

   public:
    Variable(const Context& ctx) : _ctx(ctx) {}
    Variable(const Variable& o) : _ctx(o._ctx), _shares(o._shares) {}
    Variable(Variable&& o) : _ctx(o._ctx), _shares(std::move(o._shares)) {}

    const Context& context() const { return _ctx; }
    const ShareVec& shares() const { return _shares; }

    Variable& operator=(const Variable& o) {
        _shares = o._shares;
        return *this;
    }
    Variable& operator=(Variable&& o) {
        _shares = std::move(o._shares);
        return *this;
    }
    Variable& operator=(mpc::Secret& sec) {
        _shares.clear();
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
        _shares.clear();
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

    friend Variable operator+(const Variable& lhs, const Variable& rhs) {
        Variable ret(lhs._ctx);
        auto N = lhs._ctx.N();
        for (int i = 0; i < N; i++)
            ret._shares.push_back(&(*lhs._shares[i] + *rhs._shares[i]));
        return ret;
    }

    friend Variable operator*(const Variable& lhs, mpc::Constant& c) {
        Variable ret(lhs._ctx);
        auto N = lhs._ctx.N();
        for (int i = 0; i < N; i++)
            ret._shares.push_back(&(*lhs._shares[i] * c));
        return ret;
    }
    friend Variable operator*(mpc::Constant& c, const Variable& rhs) {
        return rhs * c;
    }

    friend Variable operator*(const Variable& lhs, const Variable& rhs) {
        static size_t mulCounter = 0;
        auto& ctx = lhs._ctx;
        auto N = ctx.N();
        Variable sum(ctx);
        for (int i = 0; i < N; i++) {
            Variable inter(ctx);
            inter = *lhs._shares[i] * *rhs._shares[i];
            auto& lambda =
                *new mpc::Constant(lhs._shares.front()->context(),
                                   "mul_" + std::to_string(mulCounter) +
                                       "_lambda_" + std::to_string(i));
            if (i == 0)
                sum = inter * lambda;
            else
                sum = sum + inter * lambda;
        }
        mulCounter++;
        return sum;
    }
};
}  // namespace bgw
