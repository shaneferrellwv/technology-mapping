#include <iostream>

#include "node.h"

using namespace std;

const int infinity = 99999999;

int main(int argc, char* argv[])
{
    string fileName = argv[1];

    try
    {
        Node* andOrNotDAGRoot = Node::constructAndOrNotDAG(fileName);
        Node* nandNotDAGRoot = Node::constructNandNotDAG(andOrNotDAGRoot);
        nandNotDAGRoot = Node::simplify(nandNotDAGRoot);
        int hello = 0;
    }
    catch(const exception& e)
    {
        cerr << e.what() << endl;
    }

    return 0;
}