#include <iostream>

#include "node.h"

using namespace std;

const int infinity = 99999999;

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
        fileName = "input1.net";
    }

    try
    {
        Library* library = new Library();

        Node* andOrNotDAGRoot = Node::constructAndOrNotDAG(fileName);
        Node::outputTruthTable(andOrNotDAGRoot);
        cout << endl;
        Node* nandNotDAGRoot = Node::constructNandNotDAG(andOrNotDAGRoot);
        Node::outputTruthTable(nandNotDAGRoot);
        cout << endl;
        nandNotDAGRoot = Node::simplify(nandNotDAGRoot);
        Node::outputTruthTable(nandNotDAGRoot);
        cout << endl;

        // Find matching sturctures
        vector<string> structures = library->findMatchingStructures(nandNotDAGRoot);
        for(int i = 0; i < structures.size(); i++)
        {
            cout << structures[i] << endl;
        }
    }
    catch(const exception& e)
    {
        cerr << e.what() << endl;
    }

    return 0;
}