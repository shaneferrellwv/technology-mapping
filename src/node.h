#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include "json.hpp" // You might need a library like jsoncpp for this

using namespace std;
using json = nlohmann::json;

// forward declaration
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

static int numberOfNandNotNodes = 0;
static unordered_map<string, Node *> constructedNodes;
static unordered_map<int, Node *> nodeLookup;

string operatorToString(Operator op)
{
    switch (op)
    {
    case Operator::AND:
        return "AND";
    case Operator::OR:
        return "OR";
    case Operator::NOT:
        return "NOT";
    case Operator::NAND:
        return "NAND";
    case Operator::INPUT:
        return "INPUT";
    default:
        return "UNKNOWN";
    }
}

class Node
{
    // member variables

    // construct root from OUTPUT line
    static Node *constructRoot(vector<string> &netlistLines)
    {
        int i = 0;
        for (auto it = netlistLines.begin(); it != netlistLines.end(); ++it, i++)
        {
            string line = netlistLines[i];

            // INPUT / OUTPUT line
            if (line.find('=') == string::npos)
            {
                istringstream stream(line);
                string expr, put;

                if (!(stream >> expr >> put) || (put != "INPUT" && put != "OUTPUT"))
                    throw invalid_argument("Netlist file invalid\n");

                if (put == "OUTPUT")
                {
                    netlistLines.erase(it);

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
        int i = 0;
        for (auto it = netlistLines.begin(); it != netlistLines.end(); ++it, i++)
        {
            if (netlistLines.size() == 0)
                return root;

            string line = netlistLines[i];
            istringstream stream(line);

            // gate logic line
            if (line.find('=') != string::npos)
            {
                string name, equals, op, expr1, expr2;

                if (!(stream >> name >> equals >> op) || equals != "=")
                    throw invalid_argument("Netlist file invalid\n");

                if (name == root->name)
                {
                    netlistLines.erase(it);

                    if (op == "NOT")
                    {
                        if (!(stream >> expr1) || stream >> expr2)
                            throw invalid_argument("Netlist file invalid\n");

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

                        return root;
                    }
                    else if (op == "AND" || op == "OR")
                    {
                        if (!(stream >> expr1 >> expr2))
                            throw invalid_argument("Netlist file invalid\n");

                        root->op = op == "AND" ? Operator::AND : Operator::OR;

                        if (constructedNodes.find(expr1) == constructedNodes.end())
                        {
                            Node *left = new Node(expr1, root);
                            constructedNodes[expr1] = left;
                            root->left = constructDAGFromRoot(left, netlistLines);
                        }
                        else
                            root->left = constructedNodes[expr1];

                        if (constructedNodes.find(expr2) == constructedNodes.end())
                        {
                            Node *right = new Node(expr2, root);
                            constructedNodes[expr2] = right;
                            root->right = constructDAGFromRoot(right, netlistLines);
                        }
                        else
                            root->right = constructedNodes[expr2];

                        return root;
                    }
                }
            }
            // INPUT / OUTPUT line
            else
            {
                string name, put;

                if (!(stream >> name >> put) || (put != "INPUT" && put != "OUTPUT"))
                    throw invalid_argument("Netlist file invalid\n");

                if (put == "INPUT")
                {
                    if (name == root->name)
                    {
                        netlistLines.erase(it);

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

public:
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

    static void saveDAG(Node *root)
    {
        json j;

        // Recursive lambda function for depth-first traversal and serialization
        std::function<void(Node *, json &)> dfs = [&](Node *node, json &jsonObj)
        {
            if (!node)
            {
                std::cout << "Null node encountered, returning." << std::endl;
                return;
            }

            std::cout << "Serializing node with name: " << node->name << std::endl;
            // Serialize current node
            jsonObj["name"] = node->name;
            jsonObj["operator"] = operatorToString(node->op);

            // Recursively serialize left child if it exists
            if (node->left)
            {
                std::cout << "Traversing left child of node: " << node->name << std::endl;
                json leftChild;
                dfs(node->left, leftChild);
                jsonObj["left"] = leftChild;
            }
            else
            {
                std::cout << "Left child of node " << node->name << " is null." << std::endl;
                // If left child does not exist, set it to null in JSON
                jsonObj["left"] = nullptr;
            }

            // Recursively serialize right child if it exists
            if (node->right)
            {
                std::cout << "Traversing right child of node: " << node->name << std::endl;
                json rightChild;
                dfs(node->right, rightChild);
                jsonObj["right"] = rightChild;
            }
            else
            {
                std::cout << "Right child of node " << node->name << " is null." << std::endl;
                // If right child does not exist, set it to null in JSON
                jsonObj["right"] = nullptr;
            }
        };

        // Start the serialization from the root
        dfs(root, j);

        // Write the serialized JSON to a file
        std::ofstream file("dag.json");
        if (file.is_open())
        {
            file << j.dump(4); // Dump with indentation for readability
            file.close();
        }
        else
        {
            throw std::runtime_error("Unable to open file for writing.");
        }
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

    bool evaluate(const unordered_map<string, bool> &inputValues)
    {
        switch (op)
        {
        case Operator::AND:
            return left->evaluate(inputValues) && right->evaluate(inputValues);
        case Operator::OR:
            return left->evaluate(inputValues) || right->evaluate(inputValues);
        case Operator::NOT:
            return !left->evaluate(inputValues);
        case Operator::NAND:
            return !(left->evaluate(inputValues) && right->evaluate(inputValues));
        case Operator::INPUT:
            return inputValues.at(name);
        default:
            throw runtime_error("Unknown operator.");
        }
    }

    static void dynamicCostCalculation()
    {
        // Get to bottom of tree
        Node *currentNode = nodeLookup[0];
        while (currentNode->left != nullptr)
        {
            currentNode = currentNode->left;
        }
    }

    static void outputTruthTable(Node *root)
    {
        if (!root)
        {
            throw runtime_error("The root node cannot be null.");
        }

        // Step 1: Collect all input nodes
        vector<string> inputs;
        unordered_map<string, Node *> nodes;
        std::function<void(Node *)> collectInputs = [&](Node *node)
        {
            if (!node)
                return;
            if (node->op == Operator::INPUT && find(inputs.begin(), inputs.end(), node->name) == inputs.end())
            {
                inputs.push_back(node->name);
            }
            nodes[node->name] = node;
            collectInputs(node->left);
            collectInputs(node->right);
        };
        collectInputs(root);

        // Step 2: Generate all possible input combinations
        size_t numInputs = inputs.size();
        size_t numCombinations = 1 << numInputs; // 2^n combinations

        // Step 3: Evaluate the logical expression for each combination
        for (size_t i = 0; i < numCombinations; ++i)
        {
            unordered_map<string, bool> inputValues;
            cout << "Inputs: ";
            for (size_t j = 0; j < numInputs; ++j)
            {
                bool value = (i >> j) & 1;
                inputValues[inputs[j]] = value;
                cout << inputs[j] << "=" << value << " ";
            }

            // Evaluate the expression for the current combination
            bool result = root->evaluate(inputValues);
            cout << "Output: " << result << endl;
        }
    }

    // Helper function to check if a node is a NAND gate with both inputs the same
    static bool isNandWithSameInputs(Node *node)
    {
        return node->op == Operator::NAND && node->left == node->right;
    }

    static bool isDoubleNot(Node *node)
    {
        return node->op == Operator::NOT && node->left && node->left->op == Operator::NOT;
    }

    static Node *simplify(Node *root)
    {
        // Base case: node doesn't exist
        if (!root)
        {
            return nullptr;
        }

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
        std::reverse(sorted.begin(), sorted.end());

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

        // Print out what ids are assigned to what nodes
        for (auto node : sorted)
        {
            cout << node->name << " " << node->id << endl;
        }
    }
};