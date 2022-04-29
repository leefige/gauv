#pragma once

#include <algorithm>
#include <memory>

#include "common.hpp"
#include "node.hpp"

namespace mpc {

class GraphBase {
   protected:
    NodeVec nodes;

   public:
    GraphBase() {}
    GraphBase(NodeVec nodes) : nodes(nodes) {}
    ~GraphBase();

    NodeVec getNodes() { return nodes; };
    const NodeVec getNodes() const { return nodes; };
};

class Graph : public GraphBase {
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
