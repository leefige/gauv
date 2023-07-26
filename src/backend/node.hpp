#pragma once

#include <string>

#include "../mpcgraph/builtin.hpp"
#include "common.hpp"

namespace mpc {

class Node {
   public:
    enum NodeGenre {
        NONE = 0,

        INPUT = 100,
        OUTPUT,

        OTHERS = 200,
    };

    // Xingyu: To be honest, I think the following enum belongs to the proving algorithm. I think It's better to decouple the intermediate represetion (nodes, graphs...) and the proving algorithms.
    enum NodeState {
        UNVISITED = 0,
        VISITED,
        POTENTIAL,
        BUBBLE,
        ELIMINATED,
    } state;

    static std::string to_string(NodeGenre node_type) {
        switch (node_type) {
            case INPUT:
                return "IN";
            case OUTPUT:
                return "OUT";
            default:
                return "-";
        }
    }
    static std::string to_string(NodeState node_state) {
        switch (node_state) {
            case BUBBLE:
                return "BUBBLE";
            case POTENTIAL:
                return "P";
            default:
                return "-";
        }
    }

    Node(): hash(generateHash()) {}
    Node(std::string name, PartyDecl* party, NodeGenre type = OTHERS)
        : hash(generateHash()),
          name(name),
          party(party),
          type(type) {}
    Node(const Node& rhs)
        : guid(rhs.guid),
          hash(rhs.hash),
          state(rhs.state),
          name(rhs.name),
          party(rhs.party),
          type(rhs.type) {}
    Node(int guid) : hash(generateHash()), guid(guid) {}
    Node(int guid, std::string name, PartyDecl* party, NodeGenre type = OTHERS)
        : hash(generateHash()),
          guid(guid),
          name(name),
          party(party),
          type(type) {}
    Node(int guid, const Node& rhs)
        : state(rhs.state),
          hash(rhs.hash),
          guid(guid),
          name(rhs.name),
          party(rhs.party),
          type(rhs.type) {}

    ~Node() {}

    const std::string& getName() const { return name; }

    bool isInput() { return type == INPUT; }
    bool isOutput() { return type == OUTPUT; }

    bool markPotential() {
        if (state != BUBBLE && state != ELIMINATED/* &&
            getValidOutDegrees() == 0*/) {
            state = POTENTIAL;
            return true;
        }
        return false;
    }

    bool markEliminated() {
        state = ELIMINATED;
        return true;
    }
    bool isEliminated() const { return state == ELIMINATED; }

    /* bool checkValid() {
        // FIXME: is this intended?
        switch (isOutputOf.front()->getType()) {
            case Operator::NONE:
                return false;
            case Operator::ADD:
                return getInDegrees() == 2;
            case Operator::SUB:
                return getInDegrees() == 2;
            case Operator::MUL:
                return getInDegrees() == 2;
            case Operator::DIV:
                return getInDegrees() == 2;
            case Operator::EVAL:
                return true;
            case Operator::RECONSTRUCT:
                return true;
            default:
                return false;
        }
    } */

    std::string to_string() const {
        std::stringstream ss;
        ss << "<node[" << party->name() << ", " << to_string(type) << ", "
           << to_string(state) << "] " << name << ">";
        return ss.str();
    }

   private:
    uint64_t hash;

   public:
    const int guid = -1; // graph unique id
    std::string name;
    const PartyDecl* party;
    NodeGenre type;
};

}  // end of namespace mpc
