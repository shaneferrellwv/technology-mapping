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

    void traceback(int row)
    {
        // base case: input node
        
    }

public:
    // dp constructor
    dp()
    {
        allocate2DArray();
        solve();
        traceback(minIndex);
    }
};