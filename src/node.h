#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>

using namespace std;

// forward declaration of Node for static variables
class Node;

// operator enum declaration
enum class Operator
{
    AND,
    OR,
    NOT,
    NAND,
    INPUT
};

// static counter variables
static unsigned int numberOfNandNotNodes = 0;

// static container variables
static unordered_map<string, Node *> constructedNodes;
static unordered_map<int, Node *> nodeLookup;

struct Node
{
    // member variables
    Operator op;
    unsigned int id;
    Node *parent;
    Node *left, *right;
    string name;

    // Node constructor
    Node(string name, Node *parent)
    {
        this->name = name;
        this->parent = parent;
    }

    // construct root from OUTPUT line
    static Node *constructRoot(vector<string> &netlistLines)
    {
        int i = 0;
        for (auto it = netlistLines.begin(); it != netlistLines.end(); ++it, i++)
        {
            string line = netlistLines[i];

            // if line is a declaration
            if (line.find('=') == string::npos)
            {
                istringstream stream(line);

                // parts of a declaration line
                string expr, put;

                // missing INPUT or OUTPUT specifier
                if (!(stream >> expr >> put) || (put != "INPUT" && put != "OUTPUT"))
                    throw invalid_argument("Netlist file invalid\n");

                if (put == "OUTPUT")
                {
                    netlistLines.erase(it);

                    // construct root node
                    Node *root = new Node(expr, nullptr);
                    constructedNodes[expr] = root;

                    return root;
                }
            }
        }
        throw invalid_argument("Netlist file does not contain OUTPUT expression\n");
    }

    static Node *constructDAGFromRoot(Node *root, vector<string> &netlistLines)
    {
        // find line in netlist for the current node
        int i = 0;
        for (auto it = netlistLines.begin(); it != netlistLines.end(); ++it, i++)
        {
            // base case: all lines parsed
            if (netlistLines.size() == 0)
                return root;

            string line = netlistLines[i];
            istringstream stream(line);

            // two kinds of lines: definition and declaration
            if (line.find('=') != string::npos)
            {
                // tokens in a definition
                string name, equals, op, expr1, expr2;

                if (!(stream >> name >> equals >> op) || equals != "=")
                    throw invalid_argument("Netlist file invalid\n");

                // if this line is the definition of current expression
                if (name == root->name)
                {
                    netlistLines.erase(it);

                    // NOT gate
                    if (op == "NOT")
                    {
                        // wrong number of arguments
                        if (!(stream >> expr1) || stream >> expr2)
                            throw invalid_argument("Netlist file invalid\n");

                        // construct child
                        root->op = Operator::NOT;
                        if (constructedNodes.find(expr1) == constructedNodes.end())
                        {
                            Node *left = new Node(expr1, root);
                            constructedNodes[expr1] = left;
                            root->left = constructDAGFromRoot(left, netlistLines);
                        }
                        else
                            root->left = constructedNodes[expr1];
                        root->right = nullptr;
                    }
                    // AND & OR gate
                    else if (op == "AND" || op == "OR")
                    {
                        // wrong number of arguments
                        if (!(stream >> expr1 >> expr2))
                            throw invalid_argument("Netlist file invalid\n");

                        // construct AND or OR node
                        root->op = op == "AND" ? Operator::AND : Operator::OR;

                        // construct left child
                        if (constructedNodes.find(expr1) == constructedNodes.end())
                        {
                            Node *left = new Node(expr1, root);
                            constructedNodes[expr1] = left;
                            root->left = constructDAGFromRoot(left, netlistLines);
                        }
                        else
                            root->left = constructedNodes[expr1];

                        // construct right child
                        if (constructedNodes.find(expr2) == constructedNodes.end())
                        {
                            Node *right = new Node(expr2, root);
                            constructedNodes[expr2] = right;
                            root->right = constructDAGFromRoot(right, netlistLines);
                        }
                        else
                            root->right = constructedNodes[expr2];
                    }

                    // return parent of newly-constructed node
                    return root;
                }
            }
            else // line is a declaration
            {
                // tokens in a declaration
                string name, put;

                // wrong number of arguments
                if (!(stream >> name >> put) || (put != "INPUT" && put != "OUTPUT"))
                    throw invalid_argument("Netlist file invalid\n");

                if (put == "INPUT")
                {
                    // if this line is the declaration of the current expression
                    if (name == root->name)
                    {
                        netlistLines.erase(it);

                        // construct INPUT node
                        root->op = Operator::INPUT;
                        root->left = nullptr;
                        root->right = nullptr;

                        return root;
                    }
                }
                else
                    throw invalid_argument("Netlist cannot contain 2 OUTPUT declarations\n");
            }
        }
        throw invalid_argument("Netlist file contains invalid expression declaration\n");
    }

    static Node *constructAndOrNotDAG(string filePath)
    {
        ifstream netlist("../input/" + filePath);

        // check if file exists
        if (!netlist)
            throw invalid_argument("Unable to open input/" + filePath + "\n");

        // get netlist contents
        string line;
        vector<string> netlistLines;
        while (getline(netlist, line))
            netlistLines.push_back(line);

        // construct root from OUTPUT line. Purpose is to find output of the system and place at the top of the graph
        Node *root = constructRoot(netlistLines);

        // construct DAG from root
        return constructDAGFromRoot(root, netlistLines);
    }

    static Node *constructNandNotDAG(Node *root)
    {
        if (!root)
            return nullptr;

        // Process left and right children first
        Node *left = constructNandNotDAG(root->left);
        Node *right = constructNandNotDAG(root->right);
        Node *nandNode = nullptr;

        // Transform OR gate: OR(a, b) = NAND(NOT(a), NOT(b))
        if (root->op == Operator::OR)
        {
            Node *leftNot = new Node(root->left->name + "NOT", nullptr);
            leftNot->op = Operator::NOT;
            leftNot->left = left;
            leftNot->right = nullptr;

            Node *rightNot = new Node(root->right->name + "NOT", nullptr);
            rightNot->op = Operator::NOT;
            rightNot->left = right;
            rightNot->right = nullptr;

            root->op = Operator::NAND;
            root->left = leftNot;
            root->right = rightNot;
        }
        // Transform AND gate: AND(a, b) = NOT(NAND(a, b))
        else if (root->op == Operator::AND)
        {
            nandNode = new Node(root->name + "NAND", nullptr);
            nandNode->op = Operator::NAND;
            nandNode->left = left;
            nandNode->right = right;

            root->op = Operator::NOT;
            root->left = nandNode;
            root->right = nullptr; // NOT gates only have a single input
        }

        // For NOT gates and inputs, just update the left and right pointers
        else
        {
            root->left = left;
            root->right = right;
        }

        return root;
    }

    // utility function to check if a node is a NAND gate with both inputs the same
    static bool isNandWithSameInputs(Node *node)
    {
        return node->op == Operator::NAND && node->left == node->right;
    }

    // utility function to check if a NOT node has a NOT child
    static bool isDoubleNot(Node *node)
    {
        return node->op == Operator::NOT && node->left && node->left->op == Operator::NOT;
    }

    static Node *simplify(Node *root)
    {
        // Base case: node doesn't exist
        if (!root)
            return nullptr;

        // Recursively simplify children
        root->left = simplify(root->left);
        root->right = simplify(root->right);

        // Simplify double NOTs
        if (isDoubleNot(root))
        {
            Node *childOfNot = root->left->left;

            // If the root is the top node, we don't have to worry about the parent
            if (!root->parent)
            {
                delete root->left; // Free the immediate NOT gate
                delete root;       // Free the top NOT gate
                return childOfNot; // Return the child
            }
            else
            {
                // If the root is not the top node, rewire the parent to bypass the double NOT
                if (root->parent->left == root)
                    root->parent->left = childOfNot;
                if (root->parent->right == root)
                    root->parent->right = childOfNot;
                childOfNot->parent = root->parent;
                delete root->left; // Free the immediate NOT gate
                delete root;       // Free the current root
                return childOfNot; // Return the child
            }
        }

        // Simplify NAND gates with the same inputs
        if (isNandWithSameInputs(root))
        {
            root->op = Operator::NOT; // Convert the NAND to a NOT
            delete root->right;       // Free the redundant right child
            root->right = nullptr;    // Set the right child to nullptr
        }

        return root;
    }

    // depth first search for ordering nodes from OUTPUT node
    static void dfs(Node *node, std::vector<Node *> &sorted)
    {
        if (!node)
            return;
        if (node->left)
            dfs(node->left, sorted);
        if (node->right)
            dfs(node->right, sorted);

        if (node->op != Operator::INPUT)
            numberOfNandNotNodes++;

        sorted.push_back(node);
    }

    static void topologicalSortAndAssignIds(Node *root)
    {
        vector<Node *> sorted;
        dfs(root, sorted);

        // reverse the order to get topological sort
        reverse(sorted.begin(), sorted.end());

        // assign IDs
        int id = numberOfNandNotNodes;
        for (auto node : sorted)
        {
            if (!node->left && !node->right)
                node->id = 0; // leaf node
            else
            {
                nodeLookup[id] = node;
                node->id = id--;
            }
        }
    }
};