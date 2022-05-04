#pragma once

#include <algorithm>
#include <memory>

#include "common.hpp"
#include "node.hpp"
#include "../mpcgraph/builtin.hpp"

namespace mpc {

class GraphBase {
   protected:
    NodeVec nodes;
    OpVec edges;

   public:
    GraphBase() {}
    GraphBase(NodeVec nodes) : nodes(nodes) {}
    ~GraphBase();

    NodeVec getNodes() { return nodes; };
    const NodeVec getNodes() const { return nodes; };
};

class Graph : public GraphBase {
    // frontend -> backend map
    std::unordered_map<const Expression*, Node*> frontBackMap;
   public:
    Graph() {}
    Graph(NodeVec nodes) : GraphBase(nodes) {}

    Node* importFrontend(const Expression* exp);
};

class SubGraph : public GraphBase {
   public:
    SubGraph() {}
    SubGraph(NodeVec nodes) : GraphBase(nodes) {}
    SubGraph(const SubGraph& rhs) : SubGraph(rhs.nodes) {}
};

}  // end of namespace mpc
