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

    mpc_context(const mpc_context&) = delete;
    mpc_context(mpc_context&&) = delete;
    mpc_context& operator=(const mpc_context&) = delete;
    mpc_context& operator=(mpc_context&&) = delete;

    struct party_queue {
        std::reference_wrapper<const P_T> party;
        std::unordered_map<uint32_t, std::deque<S_T>> queues;
        party_queue(const P_T& p, const PS_T& ps) : party(p)
        {
            // insert a queue for each party: private channels
            for (const P_T& pj : ps) {
                queues.insert(std::make_pair(pj.hash(), std::deque<S_T>()));
            }
        }
    };

    const PS_T _parties;
    std::unordered_map<uint32_t, party_queue> _msgs;

    mpc_context(const PS_T& parties) noexcept : _parties(parties)
    {
        for (const P_T& p : _parties) {
            if (_msgs.find(p.hash()) != _msgs.end()) {
                throw party_duplicated<BASE>(*(_msgs.find(p.hash())), p);
            }
            register_party(p);
        }
    }

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

        _msgs.at(receiver.hash()).queue.push_back(msg);
    }

    std::optional<S_T> receive(const P_T& receiver)
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

private:
    void register_party(P_T& party)
    {
        _msgs.insert(std::make_pair(party.hash(), party_queue(party, _parties)));
        party.register_context(get_context());
    }
};

} /* namespace mpc */
