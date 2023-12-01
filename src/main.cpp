#include <iostream>

#include "dp.h"
#include "dag.h"

using namespace std;

int main(int argc, char* argv[])
{
    // user input for netlist file
    string fileName = "";
    cout << "Enter the name of the file containing the netlist: ";
    getline(cin, fileName);

    // if no file name is given, use the default input/input1.net
    if(fileName == "")
        fileName = "input1.net";

    try
    {
        dag dag(fileName);
        dp();
    }
    catch(const exception& e)
    {
        cout << e.what() << endl;
    }

    return 0;
}