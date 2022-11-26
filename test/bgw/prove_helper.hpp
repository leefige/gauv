#pragma once

#include <chrono>

#include "../../src/backend/builtin.hpp"

using namespace mpc;
using namespace std;

inline void prove_by_potential(Graph& graph, int verbose = 3) {
    graph.computePotential = true;
    graph.initOutputNodes();
    graph.initSearchState();
    cout << "Initial graph has " << graph.nodeSize() << " nodes, "
         << graph.edgeSize() << " edges" << endl;
    cout << "Initial potential: " << Graph::to_string(graph.potential())
         << endl;
    if (verbose >= 3) cout << endl << "Initial graph:" << endl << graph << endl;
    auto begin_time = chrono::high_resolution_clock::now();
    bool proved = graph.tryProvingByPotential();
    auto end_time = chrono::high_resolution_clock::now();
    cout << endl << "Proved? " << std::boolalpha << proved << endl;
    cout << "Time: " << (end_time - begin_time).count() / 1e9 << endl;
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

inline void prove_by_hint(Graph& graph, int verbose = 3) {
    graph.initOutputNodes();
    graph.initSearchState();
    cout << "Initial graph has " << graph.nodeSize() << " nodes, "
         << graph.edgeSize() << " edges" << endl;
    graph.computePotential = true;
    cout << "Initial potential: " << Graph::to_string(graph.potential())
         << endl;
    if (verbose >= 3) cout << endl << "Initial graph:" << endl << graph << endl;
    graph.computePotential = false;
    auto begin_time = chrono::high_resolution_clock::now();
    bool proved = graph.tryProvingByHint();
    auto end_time = chrono::high_resolution_clock::now();
    cout << endl << "Proved? " << std::boolalpha << proved << endl;
    cout << "Time: " << (end_time - begin_time).count() / 1e9 << endl;
    cout << "Final graph has " << graph.nodeSize() << " nodes, "
         << graph.edgeSize() << " edges" << endl;
    graph.computePotential = true;
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
