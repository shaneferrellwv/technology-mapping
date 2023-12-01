#include "node.h"

using namespace std;

// directed acyclic graph to represent logic expresssion in provided netlist
struct dag
{
    // dag constructor
    dag(string fileName)
    {
        Node* andOrNotDAGRoot = Node::constructAndOrNotDAG(fileName);
        Node* nandNotDAGRoot = Node::constructNandNotDAG(andOrNotDAGRoot);
        nandNotDAGRoot = Node::simplify(nandNotDAGRoot);
        Node::topologicalSortAndAssignIds(nandNotDAGRoot);
    }
};