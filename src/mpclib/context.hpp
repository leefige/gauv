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
    using P_T = par<BASE>;
    using PS_T = parset<BASE>;
    using S_T = share<BASE>;

    mpc_context() noexcept = default;

    mpc_context(const mpc_context&) = delete;
    mpc_context(mpc_context&&) = delete;
    mpc_context& operator=(const mpc_context&) = delete;
    mpc_context& operator=(mpc_context&&) = delete;

    struct party_queues {
        std::reference_wrapper<const P_T> party;
        std::unordered_map<uint32_t, std::deque<S_T>> queues;
        party_queues(const P_T& p, const PS_T& ps) : party(p)
        {
            // insert a queue for each party: private channels
            for (const P_T& pj : ps) {
                queues.insert(std::make_pair(pj.hash(), std::deque<S_T>()));
            }
        }
    };

    std::unordered_map<uint32_t, party_queues> _msgs;

public:
    static std::shared_ptr<mpc_context<BASE>> get_context()
    {
        static std::shared_ptr<mpc_context<BASE>> singleton(new mpc_context<BASE>());
        return singleton;
    }

    void send(const P_T& sender, const P_T& receiver, const S_T& msg)
    {
        if (receiver != msg.party()) {
            throw party_missmatch(receiver, msg.party());
        }

        if (_msgs.find(receiver.hash()) == _msgs.end()) {
            throw party_nonexist();
        }

        auto& receiver_queues = _msgs.at(receiver.hash()).queues;
        if (receiver_queues.find(sender.hash()) == receiver_queues.end()) {
            throw party_nonexist();
        }

        receiver_queues.at(sender.hash()).push_back(msg);
    }

    std::optional<S_T> receive(const P_T& receiver, const P_T& sender)
    {
        if (_msgs.find(receiver.hash()) == _msgs.end()) {
            throw party_nonexist();
        }

        auto& receiver_queues = _msgs.at(receiver.hash()).queues;
        if (receiver_queues.find(sender.hash()) == receiver_queues.end()) {
            throw party_nonexist();
        }
        auto& dst_queue = receiver_queues.at(sender.hash());

        if (dst_queue.size() <= 0) {
            return std::nullopt;
        }

        auto& ret = dst_queue.front();
        dst_queue.pop_front();
        return ret;
    }

    void register_parties(const PS_T& parties)
    {
        for (P_T& p : parties) {
            const auto& it = _msgs.find(p.hash());
            if (it != _msgs.end()) {
                throw party_duplicated<BASE>(it->second.party, p);
            }
            _msgs.insert(std::make_pair(p.hash(), party_queues(p, parties)));
            p.register_context(get_context());
        }
    }
};

} /* namespace mpc */
