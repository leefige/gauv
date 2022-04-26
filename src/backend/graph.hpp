#pragma once

#include "common.hpp"
#include "node.hpp"

#include <memory>

namespace mpc {

class GraphBase {
protected:
    NodeVec nodes;

public:
    ~GraphBase();

    NodeVec getNodes() { return nodes; };
    const NodeVec getNodes() const { return nodes; };

};

class Graph : public GraphBase {
public:
    Graph() {}
    Graph(NodeVec nodes) : nodes(nodes) {}
};

class SubGraph : public GraphBase {
public:
    SubGraph() {}
    SubGraph(NodeVec nodes) : nodes(nodes) {}
    SubGraph(const SubGraph &rhs) : SubGraph(rhs.nodes) {}
};

}  // end of namespace mpc
