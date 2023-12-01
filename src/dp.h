#include <fstream>
#include <vector>
#include <unordered_set>

#include "node.h"

using namespace std;

const int infinity = 99999999;

class dp
{
    // member variables
    int **A, minCost, minIndex;
    unordered_set<string> definedExpressions;

    // utility function to allocate 2D dp array
    void allocate2DArray()
    {
        A = new int*[numberOfNandNotNodes + 1];

        for (unsigned int i = 0; i <= numberOfNandNotNodes; i++)
            A[i] = new int[8];
    }

    // utility function to delete 2D dp array
    void deallocate2DArray()
    {
        for (unsigned int i = 0; i <= numberOfNandNotNodes; i++)
            delete[] A[i];

        delete[] A;
    }

    // utlitity function to retrieve minimum element from row
    int getMin(int i)
    {
        int min = infinity;
        for (int j = 0; j <= 7; j++)
            if (A[i][j] < min)
                min = A[i][j];
        
        return min;
    }

    // utlitity function to retrieve column index of minimum element from row
    int getMinIndex(int i)
    {
        int column;
        int min = infinity;

        for (int j = 0; j <= 7; j++)
            if (A[i][j] < min)
            {
                min = A[i][j];
                column = j;
            }

        return column;
    }

    // solves problem using bottom-up dynamic programming for recurrence relation
    void solve()
    {
        for (unsigned int i = 0; i <= numberOfNandNotNodes; i++)
        {
            for (int j = 0; j <= 7; j++)
            {
                if (i == 0)
                    A[i][j] = 0;
                else if (j == 0)
                    A[i][j] = infinity;
                else if (j == 1 && nodeLookup[i]->op == Operator::NOT)
                    A[i][j] = 2 + getMin(nodeLookup[i]->left->id);
                else if (j == 2 && nodeLookup[i]->op == Operator::NAND)
                    A[i][j] = 3 + getMin(nodeLookup[i]->left->id)
                                + getMin(nodeLookup[i]->right->id);
                else if (j == 3 && nodeLookup[i]->op == Operator::NOT
                                && nodeLookup[i]->left->op == Operator::NAND)
                    A[i][j] = 4 + getMin(nodeLookup[i]->left->left->id)
                                + getMin(nodeLookup[i]->left->right->id);
                else if (j == 4 && nodeLookup[i]->op == Operator::NOT 
                                && nodeLookup[i]->left->op == Operator::NAND 
                                && nodeLookup[i]->left->left->op == Operator::NOT 
                                && nodeLookup[i]->left->right->op == Operator::NOT)
                    A[i][j] = 6 + getMin(nodeLookup[i]->left->left->left->id)
                                + getMin(nodeLookup[i]->left->right->left->id);
                else if (j == 5 && nodeLookup[i]->op == Operator::NAND 
                                && nodeLookup[i]->left->op == Operator::NOT 
                                && nodeLookup[i]->right->op == Operator::NOT)
                    A[i][j] = 6 + getMin(nodeLookup[i]->left->left->id)
                                + getMin(nodeLookup[i]->right->left->id);
                else if (j == 6 && nodeLookup[i]->op == Operator::NOT 
                                && nodeLookup[i]->left->op == Operator::NAND 
                                && nodeLookup[i]->left->left->op == Operator::NAND 
                                && nodeLookup[i]->left->right->op == Operator::NOT)
                    A[i][j] = 7 + getMin(nodeLookup[i]->left->left->left->id)
                                + getMin(nodeLookup[i]->left->left->right->id)
                                + getMin(nodeLookup[i]->left->right->left->id);
                else if (j == 7 && nodeLookup[i]->op == Operator::NOT 
                                && nodeLookup[i]->left->op == Operator::NAND 
                                && nodeLookup[i]->left->left->op == Operator::NAND 
                                && nodeLookup[i]->left->right->op == Operator::NAND)
                    A[i][j] = 7 + getMin(nodeLookup[i]->left->left->left->id)
                                + getMin(nodeLookup[i]->left->left->right->id) 
                                + getMin(nodeLookup[i]->left->right->left->id) 
                                + getMin(nodeLookup[i]->left->right->right->id);
                else
                    A[i][j] = infinity;
            }
        }
        minCost = getMin(numberOfNandNotNodes);
        minIndex = getMinIndex(numberOfNandNotNodes);
    }

    // utility to determine if an expression has been defined
    bool isDefined(Node* node)
    {
        string expressionName;
        if (node->op == Operator::INPUT)
            expressionName = node->name;
        else
            expressionName = "t" + node->id;

        if (definedExpressions.find(expressionName) != definedExpressions.end())
            return true;
        else
            return false;
    }

    // places defintion of current node into netlist output vector
    string define(Node* root, vector<string>& output)
    {
        // OUTPUT expression must be defined first
        static bool isOutputDefined = false;
        if (!isOutputDefined)
        {
            isOutputDefined = true;
            definedExpressions.insert(root->name);
            output.push_back(root->name + " OUTPUT");
            return define(root, output);
        }

        // base case: defined expression
        if (isDefined(root))
        {
            if (root->op == Operator::INPUT)
                return root->name;
            else
                return "t" + root->id;
        }

        // else write definition
        switch (getMinIndex(root->id))
        {
            case 0: // INPUT
            {
                output.push_back(root->name + " INPUT");
                // definedExpressions.insert(root->name); // try commenting this out seems unnecessary
                return root->name;
            }
            case 1: // NOT gate definition
            {
                Node* child1 = root->left;
                string expr1 = define(child1, output);
                definedExpressions.insert(expr1);
                if (root->id == numberOfNandNotNodes)
                    output.push_back(root->name + " = NOT " + expr1);
                else
                    output.push_back("t" + to_string(root->id) + " = NOT " + expr1);
                return "t" + to_string(root->id);
            }
            case 2: // NAND2 gate definition
            {
                Node* child1 = root->left;
                Node* child2 = root->right;
                string expr1 = define(child1, output);
                string expr2 = define(child2, output);
                definedExpressions.insert(expr1);
                definedExpressions.insert(expr2);
                if (root->id == numberOfNandNotNodes)
                    output.push_back(root->name + " = NAND2 " + expr1 + " " + expr2);
                else
                    output.push_back("t" + to_string(root->id) + " = NAND2 " + expr1 + " " + expr2);
                return "t" + to_string(root->id);
            }
            case 3: // AND2 gate definition
            {
                Node* child1 = root->left->left;
                Node* child2 = root->left->right;
                string expr1 = define(child1, output);
                string expr2 = define(child2, output);
                definedExpressions.insert(expr1);
                definedExpressions.insert(expr2);
                if (root->id == numberOfNandNotNodes)
                    output.push_back(root->name + " = AND2 " + expr1 + " " + expr2);
                else
                    output.push_back("t" + to_string(root->id) + " = AND2 " + expr1 + " " + expr2);
                return "t" + to_string(root->id);
            }
            case 4: // NOR2 gate definition
            {
                Node* child1 = root->left->left->left;
                Node* child2 = root->left->right->left;
                string expr1 = define(child1, output);
                string expr2 = define(child2, output);
                definedExpressions.insert(expr1);
                definedExpressions.insert(expr2);
                if (root->id == numberOfNandNotNodes)
                    output.push_back(root->name + " = NOR2 " + expr1 + " " + expr2);
                else
                    output.push_back("t" + to_string(root->id) + " = NOR2 " + expr1 + " " + expr2);
                return "t" + to_string(root->id);
            }
            case 5: // OR2 gate definition
            {
                Node* child1 = root->left->left;
                Node* child2 = root->right->left;
                string expr1 = define(child1, output);
                string expr2 = define(child2, output);
                definedExpressions.insert(expr1);
                definedExpressions.insert(expr2);
                if (root->id == numberOfNandNotNodes)
                    output.push_back(root->name + " = OR2 " + expr1 + " " + expr2);
                else
                    output.push_back("t" + to_string(root->id) + " = OR2 " + expr1 + " " + expr2);
                return "t" + to_string(root->id);
            }
            case 6: // AO121 gate definition
            {
                Node* child1 = root->left->left->left;
                Node* child2 = root->left->left->right;
                Node* child3 = root->left->right->left;
                string expr1 = define(child1, output);
                string expr2 = define(child2, output);
                string expr3 = define(child3, output);
                definedExpressions.insert(expr1);
                definedExpressions.insert(expr2);
                definedExpressions.insert(expr3);
                if (root->id == numberOfNandNotNodes)
                    output.push_back(root->name + " = AO121 " + expr1 + " " + expr2 + " " + expr3);
                else
                    output.push_back("t" + to_string(root->id) + " = AO121 " + expr1 + " " + expr2 + " " + expr3);
                return "t" + to_string(root->id);
            }
            case 7: // AO122 gate definition
            {
                Node* child1 = root->left->left->left;
                Node* child2 = root->left->left->right;
                Node* child3 = root->left->right->left;
                Node* child4 = root->left->right->right;
                string expr1 = define(child1, output);
                string expr2 = define(child2, output);
                string expr3 = define(child3, output);
                string expr4 = define(child4, output);
                definedExpressions.insert(expr1);
                definedExpressions.insert(expr2);
                definedExpressions.insert(expr3);
                definedExpressions.insert(expr4);
                if (root->id == numberOfNandNotNodes)
                    output.push_back(root->name + " = AO122 " + expr1 + " " + expr2 + " " + expr3 + " " + expr4);
                else
                    output.push_back("t" + to_string(root->id) + " = AO122 " + expr1 + " " + expr2 + " " + expr3 + " " + expr4);
                return "t" + to_string(root->id);
            }
        }
        return string();
    }

    void printSolution()
    {
        if (numberOfNandNotNodes == 0)
        {
            ofstream netlist("output.net");
            netlist.close();
            cout << "Mimimum cost: 0" << endl;
            return;
        }

        vector<string> output;
        define(nodeLookup[numberOfNandNotNodes], output);
        ofstream netlist("output.net");
        for (long long unsigned int i = 0; i < output.size(); i++)
        {
            netlist << output[i];
            if (i < output.size() - 1)
                netlist << endl;
        }
        netlist.close();

        // print mimumum cost
        cout << "Minimum cost: " << minCost << endl;
    }

public:
    // dp constructor
    dp()
    {
        allocate2DArray();
        solve();
        printSolution();
    }
};