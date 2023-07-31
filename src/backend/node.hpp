#pragma once

#include <string>
#include <functional>

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


    // Xingyu: To be honest, I think the following enum belongs to the proving algorithm. I think It's better to decouple the intermediate represetion (nodes, graphs...) and the proving algorithms.
    // enum NodeState {
    //     UNVISITED = 0,
    //     VISITED,
    //     POTENTIAL,
    //     BUBBLE,
    //     ELIMINATED,
    // } state;
    // static std::string to_string(NodeState node_state) {
    //     switch (node_state) {
    //         case BUBBLE:
    //             return "BUBBLE";
    //         case POTENTIAL:
    //             return "P";
    //         default:
    //             return "-";
    //     }
    // }

    Node() {}
    Node(std::string name, const PartyDecl*& party, NodeGenre type = OTHERS)
        : name(name),
          party(party),
          type(type) {}
    Node(const Node& rhs)
        : // state(rhs.state),
          guid(rhs.guid),
          hash(rhs.hash),
          name(rhs.name),
          party(rhs.party),
          type(rhs.type) {}
    Node(int guid) : guid(guid), hash(std::hash<int>{}(guid)) {}
    Node(int guid, std::string name, const PartyDecl*& party, NodeGenre type = OTHERS)
        : guid(guid),
          hash(std::hash<int>{}(guid)),
          name(name),
          party(party),
          type(type) {}
    Node(int guid, const Node& rhs)
        : // state(rhs.state),
          guid(guid),
          hash(rhs.hash),
          name(rhs.name),
          party(rhs.party),
          type(rhs.type) {}

    ~Node() {}

    const std::string& getName() const { return name; }

    bool isInput() { return type == INPUT; }
    bool isOutput() { return type == OUTPUT; }

    // bool markPotential() {
    //     if (state != BUBBLE && state != ELIMINATED/* &&
    //         getValidOutDegrees() == 0*/) {
    //         state = POTENTIAL;
    //         return true;
    //     }
    //     return false;
    // }

    // bool markEliminated() {
    //     state = ELIMINATED;
    //     return true;
    // }
    // bool isEliminated() const { return state == ELIMINATED; }

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
        ss << "<node[" << party->name() << ", " << to_string(type) << ", " << guid << "] " << name << ">";
        return ss.str();
    }

   public:
    const int guid = -1; // graph unique id
    const size_t hash = 0;
    std::string name;
    const PartyDecl* party;
    NodeGenre type;
};

}  // end of namespace mpc
