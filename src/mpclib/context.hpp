#pragma once

#include "defs.hpp"

#include <deque>
#include <functional>
#include <unordered_map>
#include <optional>

#include <iostream>

namespace mpc {

/* NOT thread-safe! */
template<base_t BASE>
class mpc_context {
    using F = field<BASE>;
    using P = par<BASE>;
    using S = share<BASE>;

    mpc_context() noexcept = default;

    mpc_context(const mpc_context&) = delete;
    mpc_context(mpc_context&&) = delete;
    mpc_context& operator=(const mpc_context&) = delete;
    mpc_context& operator=(mpc_context&&) = delete;

public:
    struct party_queue {
        std::reference_wrapper<const P> party;
        std::deque<S> queue;
        party_queue(const P& p) : party(p), queue() {}
    };

private:
    std::unordered_map<uint32_t, party_queue> _msgs;

public:
    static std::shared_ptr<mpc_context<BASE>> get_context()
    {
        static std::shared_ptr<mpc_context<BASE>> singleton(new mpc_context<BASE>());
        return singleton;
    }

    void register_party(P& party)
    {
        _msgs.insert(std::make_pair(party.hash(), party_queue(party)));
        party.register_context(get_context());
    }

    void send(const P& sender, const P& receiver, const S& msg)
    {
        if (receiver != msg.party()) {
            throw party_missmatch(receiver, msg.party());
        }

        if (_msgs.find(receiver.hash()) == _msgs.end()) {
            throw party_nonexist();
        }

        _msgs.at(receiver.hash()).queue.push_back(msg);
    }

    std::optional<S> receive(const P& receiver)
    {
        if (_msgs.find(receiver.hash()) == _msgs.end()) {
            throw party_nonexist();
        }

        auto& dst = _msgs.at(receiver.hash()).queue;
        if (dst.size() <= 0) {
            return std::nullopt;
        }

        auto& ret = dst.front();
        dst.pop_front();
        return ret;
    }
};

} /* namespace mpc */
