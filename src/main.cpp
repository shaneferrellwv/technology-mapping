#include <iostream>

#include "dp.h"

using namespace std;

int main(int argc, char* argv[])
{
    // take input from user or if empty default to input/input1.net which is a txt
    // file containing the netlist
    string fileName = "";
    cout << "Enter the name of the file containing the netlist: ";
    getline(cin, fileName);

    // if no file name is given, use the default input/input1.net
    if(fileName == "")
    {
        fileName = "input5.net";
    }

    try
    {
        Node* andOrNotDAGRoot = Node::constructAndOrNotDAG(fileName);
        Node::outputTruthTable(andOrNotDAGRoot);
        cout << endl;
        Node* nandNotDAGRoot = Node::constructNandNotDAG(andOrNotDAGRoot);
        Node::outputTruthTable(nandNotDAGRoot);
        cout << endl;
        nandNotDAGRoot = Node::simplify(nandNotDAGRoot);
        // save the node lookup table
        Node::saveDAG(nandNotDAGRoot);
        Node::outputTruthTable(nandNotDAGRoot);
        cout << endl;
        Node::topologicalSortAndAssignIds(nandNotDAGRoot);
        cout << endl;
        dp();
        cout << endl;
    }
    catch(const exception& e)
    {
        cout << e.what() << endl;
    }

    return 0;
}