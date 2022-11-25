#pragma once

#include "../../src/backend/builtin.hpp"

using namespace mpc;
using namespace std;

inline void prove_helper(Graph& graph, int verbose = 3) {
    graph.initOutputNodes();
    graph.initSearchState();
    cout << "Initial graph has " << graph.nodeSize() << " nodes, "
         << graph.edgeSize() << " edges" << endl;
    cout << "Initial potential: " << Graph::to_string(graph.potential())
         << endl;
    if (verbose >= 3) cout << endl << "Initial graph:" << endl << graph << endl;
    bool proved = graph.tryProvingByPotential();
    cout << endl << "Proved? " << std::boolalpha << proved << endl;
    cout << "Final graph has " << graph.nodeSize() << " nodes, "
         << graph.edgeSize() << " edges" << endl;
    cout << "Final potential: " << Graph::to_string(graph.potential()) << endl;
    if (verbose >= 3) cout << "Final graph:" << endl << graph << endl;
    if (verbose >= 2) {
        cout << endl << "Transform history:" << endl;
        for (auto& entry : graph.transformTape) {
            cout << entry.node->getName() << " " << Graph::to_string(entry.type)
                 << " " << Graph::to_string(entry.potential) << endl;
        }
    }
}
