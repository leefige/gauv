#pragma once

#include "common.hpp"
#include "context.hpp"

namespace bgw {
class Variable {
    const Context& _ctx;
    ShareVec _shares;

public:
    Variable(const Context& ctx) : _ctx(ctx) {}
    // This Shamir sharing is input, so we do not have a secret explicitly.
    Variable(const Context& bgw_ctx, mpc::Context &mpc_ctx, mpc::Type* type) : _ctx(bgw_ctx) {
        for (auto p : _ctx.parties()) {
            _shares.push_back(&mpc::Share::gen_share(mpc_ctx, type, p));
        }
    }
    // Some party has a secret, and creates a Shamir sharing to share its secret.
    Variable(const Context& ctx, mpc::Expression& s) : _ctx(ctx) {
        // Xingyu: we'd better to check the type of s here. It must be a number (not a polynomial).
        auto& poly =
            mpc::Poly::gen_poly(s.context(), s.party(), s, _ctx.T());
        for (auto p : _ctx.parties()) {
            auto& s = poly.eval(*p);
            if (p == s.party())
                _shares.push_back(&s);
            else
                _shares.push_back(&s.transfer(p));
        }
    }
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
    Variable& operator=(mpc::Expression& s) {
        // Xingyu: we'd better to check the type of s here. It must be a number (not a polynomial).
        auto& poly =
            mpc::Poly::gen_poly(s.context(), s.party(), s, _ctx.T());
        for (auto p : _ctx.parties()) {
            auto& s = poly.eval(*p);
            if (p == s.party())
                _shares.push_back(&s);
            else
                _shares.push_back(&s.transfer(p));
        }
        return *this;
    }

    mpc::Share& yield(mpc::PartyDecl* party, std::string name="") const {
        // Xingyu: 我感觉这个函数的返回值的类型是 Share 好像稍微有点奇怪🤔
        std::vector<mpc::Expression*> transferred;
        for (auto s : _shares) {
            if (s->party() == party) {
                transferred.push_back(s);
            } else {
                transferred.push_back(&s->transfer(party));
            }
        }
        return mpc::Share::reconstruct(transferred, party, name);
    }

    friend Variable operator+(const Variable& lhs, const Variable& rhs) {
        static size_t addCounter = 0;
        Variable ret(lhs._ctx);
        auto N = lhs._ctx.N();
        for (unsigned i = 0; i < N; i++)
            ret._shares.push_back(&(*lhs._shares[i] + *rhs._shares[i]));
        addCounter++;
        spdlog::trace("addCounter: {}", addCounter);
        return ret;
    }

    friend Variable operator*(const Variable& lhs, mpc::Constant& c) {
        static size_t constMulCounter = 0;
        Variable ret(lhs);
        auto N = lhs._ctx.N();
        for (unsigned i = 0; i < N; i++)
            ret._shares[i] = &(*lhs._shares[i] * c);
        constMulCounter++;
        spdlog::trace("constMulCounter: {}", constMulCounter);
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
        for (unsigned i = 0; i < N; i++) {
            Variable inter(ctx, *lhs._shares[i] * *rhs._shares[i]);
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
        spdlog::trace("mulCounter: {}", mulCounter);
        return sum;
    }
};
}  // namespace bgw
