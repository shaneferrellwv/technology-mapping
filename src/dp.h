#include "node.h"

const int infinity = 99999999;

class dp
{
    // member variables
    int **A, minCost, minIndex;

    // utility function to allocate 2D dp array
    void allocate2DArray()
    {
        A = new int*[numberOfNandNotNodes + 1];

        for (int i = 0; i <= numberOfNandNotNodes; i++)
            A[i] = new int[8];
    }

    // utility function to delete 2D dp array
    void deallocate2DArray()
    {
        for (int i = 0; i <= numberOfNandNotNodes; i++)
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
        for (int i = 0; i <= numberOfNandNotNodes; i++)
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

    // get the children given a cost of the symbol
    vector<Node*> getChildren(int cost, int index, int nodeIndex, int tab=0)
    {
        vector<Node*> children;
        //tab
        for(int i=0; i<tab; i++){
            cout << "\t";
        }
        if(index==6){
            // AO121 Gate
            children.push_back(nodeLookup[nodeIndex]->left->left->left);
            children.push_back(nodeLookup[nodeIndex]->left->left->right);
            children.push_back(nodeLookup[nodeIndex]->left->right->left);
            cout << "AO121 Gate" << endl;
        }
        else if(index==7){
            // AO122 Gate
            children.push_back(nodeLookup[nodeIndex]->left->left->left);
            children.push_back(nodeLookup[nodeIndex]->left->left->right);
            children.push_back(nodeLookup[nodeIndex]->left->right->left);
            children.push_back(nodeLookup[nodeIndex]->left->right->right);
            cout << "AO122 Gate" << endl;
        }
        else if(index==5){
            // OR2
            children.push_back(nodeLookup[nodeIndex]->left->left);
            children.push_back(nodeLookup[nodeIndex]->right->left);
            cout << "OR2 Gate" << endl;
        }
        else if(index==4){
            // NOR2 gate
            children.push_back(nodeLookup[nodeIndex]->left->left->left);
            children.push_back(nodeLookup[nodeIndex]->left->right->left);
            cout << "NOR2 Gate" << endl;
        }
        else if(index==3){
            // AND2 gate
            children.push_back(nodeLookup[nodeIndex]->left->left);
            children.push_back(nodeLookup[nodeIndex]->left->right);
            cout << "AND2 Gate" << endl;
        }
        else if(index==2){
            // NAND2 gate
            children.push_back(nodeLookup[nodeIndex]->left);
            children.push_back(nodeLookup[nodeIndex]->right);
            cout << "NAND2 Gate" << endl;
        }
        else if(index==1){
            // NOT Gate
            children.push_back(nodeLookup[nodeIndex]->left);
            cout << "NOT Gate" << endl;
        }
        return children;
    }

    void traceback(int row, int tab = 0)
    {
        // Get the row array
        minCost = getMin(row);
        minIndex = getMinIndex(row);
        
        // Get the children
        vector<Node*> children = getChildren(minCost, minIndex, row, tab);
        for(int i=0; i<tab; i++){
            cout << "\t";
        }
        cout << "Found node: " << nodeLookup[row]->name << endl;
        tab++;
        for (int i = 0; i < children.size(); i++)
        {   
            for(int i=0; i<tab; i++){
                cout << "\t";
            }
            cout << "Child: " << i << endl;
            if(children[i]->id == 0){
                for(int i=0; i<tab; i++){
                    cout << "\t";
                }
                cout << "Found primary input: " << children[i]->name << endl;
            }else{
                traceback(children[i]->id, tab);
            }
        }

    }

public:
    // dp constructor
    dp()
    {
        allocate2DArray();
        solve();
        // print out the minimum cost table with proper formatting and prints out inf if infinity. Also should print row label which is name of node at that index
        cout << "Minimum Cost Table" << endl;
        cout << "------------------" << endl;
        cout << "Node\t";
        for (int i = 0; i <= 7; i++)
            cout << i << "\t";
        cout << endl;
        for (int i = 0; i <= numberOfNandNotNodes; i++)
        {
            if(i==0){
                cout << "Primary Inputs\t";
            }else{
            cout << nodeLookup[i]->name << "\t";
            for (int j = 0; j <= 7; j++)
            {
                if (A[i][j] == infinity)
                    cout << "inf\t";
                else
                    cout << A[i][j] << "\t";
            }
            }
            cout << endl;
        }
        traceback(numberOfNandNotNodes);
    }
};