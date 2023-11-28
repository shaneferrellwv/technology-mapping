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

static int numberOfAndOrNotNodes = 0;
static unordered_map<string, Node *> constructedNodes;

string operatorToString(Operator op) {
    switch (op) {
        case Operator::AND:  return "AND";
        case Operator::OR:   return "OR";
        case Operator::NOT:  return "NOT";
        case Operator::NAND: return "NAND";
        case Operator::INPUT:return "INPUT";
        default:             return "UNKNOWN";
    }
}
class Node
{
    // member variables
    string name;
    unsigned int id;
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
    Node *parent;
    Node *left, *right;
    // Node constructor
    Node(string name, Node *parent)
    {
        this->name = name;
        this->parent = parent;
        numberOfAndOrNotNodes++;
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

    // static Node* constructNandNotDAG(Node* root)
    // {
    //     // base case: all nodes have been converted
    //     if (constructedNodes.find(root->name) == constructedNodes.end())
    //         return root;
    //     else
    //         constructedNodes.erase(root->name);

    //     Node *right, *left;
    //     left = root->left;
    //     right = root->right;

    //     // if not already a NAND or NOT gate, then convert to NAND and NOT gates
    //     // a AND b --> NOT(a NAND b)
    //     if (root->op == Operator::AND)
    //     {
    //         Node* nandNot = new Node(root->name + "NOT", root->parent);
    //         nandNot->op = Operator::NOT;
    //         nandNot->left = root;
    //         nandNot->right = nullptr;
    //         root->parent = nandNot;
    //         root = nandNot;
    //     }
    //     // a OR b --> (NOT a) NAND (NOT B)
    //     else if (root->op == Operator::OR)
    //     {
    //         Node* leftNot = new Node(root->left->name + "NOT", root->parent);
    //         left->parent = leftNot;
    //         leftNot->left = left;
    //         root->left = leftNot;
    //         leftNot->right = nullptr;
    //         leftNot->parent = root;
    //         leftNot->op = Operator::NOT;

    //         Node* rightNot = new Node(root->right->name + "NOT", root->parent);
    //         right->parent = rightNot;
    //         rightNot->left = right;
    //         root->right = rightNot;
    //         rightNot->right = nullptr;
    //         rightNot->parent = root;
    //         rightNot->op = Operator::NOT;

    //         root->op = Operator::NAND;
    //         root->name += "NAND";
    //     }

    //     // recursively convert children
    //     if (left)
    //         constructNandNotDAG(left);
    //     if (right)
    //         constructNandNotDAG(right);

    //     return root;
    // }

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

    static bool isDoubleNot(Node* node) {
        return node->op == Operator::NOT && node->left && node->left->op == Operator::NOT;
    }

    // Helper function to check if a node is a NAND gate with both inputs the same
    static bool isNandWithSameInputs(Node* node) {
        return node->op == Operator::NAND && node->left == node->right;
    }

    // static Node *simplify(Node *root)
    // {
    //     // base case: input node
    //     if (root->op == Operator::INPUT)
    //         return root;

    //     // if there are two NOTs in a row, simplify
    //     if (root->op == Operator::NOT && root->left->op == Operator::NOT)
    //     {
    //         if (!root->parent)
    //             return root->left->left;

    //         root->left->left->parent = root->parent;
    //         root->parent->left = root->left->left;
    //         root->left = nullptr;
    //     }

    //     // recursively simplify DAG
    //     if (root->left)
    //         simplify(root->left);
    //     if (root->right)
    //         simplify(root->right);

    //     return root;
    // }
    // Function to simplify double NOTs and NAND gates with the same inputs

    bool evaluate(const unordered_map<string, bool>& inputValues) {
        switch(op) {
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

    static void outputTruthTable(Node* root) {
        if (!root) {
            throw runtime_error("The root node cannot be null.");
        }

        // Step 1: Collect all input nodes
        vector<string> inputs;
        unordered_map<string, Node*> nodes;
        std::function<void(Node*)> collectInputs = [&](Node* node) {
            if (!node) return;
            if (node->op == Operator::INPUT && find(inputs.begin(), inputs.end(), node->name) == inputs.end()) {
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
        for (size_t i = 0; i < numCombinations; ++i) {
            unordered_map<string, bool> inputValues;
            cout << "Inputs: ";
            for (size_t j = 0; j < numInputs; ++j) {
                bool value = (i >> j) & 1;
                inputValues[inputs[j]] = value;
                cout << inputs[j] << "=" << value << " ";
            }

            // Evaluate the expression for the current combination
            bool result = root->evaluate(inputValues);
            cout << "Output: " << result << endl;
        }
    }

    static Node* simplify(Node* root) {
        if (!root) {
            // std::cout << "Reached a null node, returning." << std::endl;
            return nullptr;
        }

        // std::cout << "Simplifying node: " << root->name << " with operator: " << operatorToString(root->op) << std::endl;

        // Recursively simplify children first
        root->left = simplify(root->left);
        root->right = simplify(root->right);

        // Simplify double NOTs
        if (isDoubleNot(root)) {
            // std::cout << "Simplifying double NOT at node: " << root->name << std::endl;
            Node* childOfNot = root->left->left;
            // If the root is the top node, we don't have to worry about the parent
            if (!root->parent) {
                delete root->left; // Free the immediate NOT gate
                delete root;       // Free the top NOT gate
                return childOfNot; // Return the child
            } else {
                // If the root is not the top node, rewire the parent to bypass the double NOT
                if (root->parent->left == root) root->parent->left = childOfNot;
                if (root->parent->right == root) root->parent->right = childOfNot;
                childOfNot->parent = root->parent;
                delete root->left; // Free the immediate NOT gate
                delete root;       // Free the current root
                return childOfNot; // Return the child
            }
        }

        // Simplify NAND gates with the same inputs
        if (isNandWithSameInputs(root)) {
            // std::cout << "Simplifying NAND with same inputs at node: " << root->name << std::endl;
            root->op = Operator::NOT; // Convert the NAND to a NOT
            delete root->right;       // Free the redundant right child
            root->right = nullptr;    // Set the right child to nullptr
        }

        // Simplify other patterns if needed

        return root;
    }
};

class Library{
    private:
        Node *NOTStructure = new Node("", nullptr);
        Node *NAND2Structure = new Node("", nullptr);
        Node *AND2Structure = new Node("", nullptr);
        Node *NOR2Structure = new Node("", nullptr);
        Node *OR2Structure = new Node("", nullptr);
        Node *AOI21Structure = new Node("", nullptr);
        Node *AOI22Structure = new Node("", nullptr);

        bool areStructurallyEquivalent(Node* a, Node* b) {
            if (a == nullptr && b == nullptr) {
                return true;
            }
            if (a == nullptr || b == nullptr) {
                return false;
            }
            
            if(b->op == Operator::INPUT) {
                return true;
            }

            if (a->op != b->op) {
                return false;
            }
            return areStructurallyEquivalent(a->left, b->left) && areStructurallyEquivalent(a->right, b->right);
        }
    
    public:
        // Library constructor
        Library()
        {
            NOTStructure->op = Operator::NOT;
            Node *NOTStructureLeft = new Node("", NOTStructure);
            NOTStructureLeft->op = Operator::INPUT;
            NOTStructure->left = NOTStructureLeft;

            NAND2Structure->op = Operator::NAND;
            Node *NAND2StructureLeft = new Node("", NAND2Structure);
            Node *NAND2StructureRight = new Node("", NAND2Structure);
            NAND2StructureLeft->op = Operator::INPUT;
            NAND2StructureRight->op = Operator::INPUT;
            NAND2Structure->left = NAND2StructureLeft;
            NAND2Structure->right = NAND2StructureRight;

            AND2Structure->op = Operator::NOT;
            Node *AND2StructureLeft = new Node("", AND2Structure);
            AND2StructureLeft->op = Operator::NAND;
            Node *AND2StructureLeftLeft = new Node("", AND2StructureLeft);
            Node *AND2StructureLeftRight = new Node("", AND2StructureLeft);
            AND2StructureLeftLeft->op = Operator::INPUT;
            AND2StructureLeftRight->op = Operator::INPUT;
            AND2StructureLeft->left = AND2StructureLeftLeft;
            AND2StructureLeft->right = AND2StructureLeftRight;
            AND2Structure->left = AND2StructureLeft;

            NOR2Structure->op = Operator::NOT;
            Node *NOR2StructureLeft = new Node("", NOR2Structure);
            NOR2StructureLeft->op = Operator::NAND;
            Node *NOR2StructureLeftLeft = new Node("", NOR2StructureLeft);
            Node *NOR2StructureLeftRight = new Node("", NOR2StructureLeft);
            NOR2StructureLeftLeft->op = Operator::NOT;
            NOR2StructureLeftRight->op = Operator::NOT;
            Node *NOR2StructureLeftLeftLeft = new Node("", NOR2StructureLeftLeft);
            Node *NOR2StructureLeftRightLeft = new Node("", NOR2StructureLeftRight);
            NOR2StructureLeftLeftLeft->op = Operator::INPUT;
            NOR2StructureLeftRightLeft->op = Operator::INPUT;
            NOR2StructureLeftLeft->left = NOR2StructureLeftLeftLeft;
            NOR2StructureLeftRight->left = NOR2StructureLeftRightLeft;
            NOR2StructureLeft->left = NOR2StructureLeftLeft;
            NOR2StructureLeft->right = NOR2StructureLeftRight;
            NOR2Structure->left = NOR2StructureLeft;



            OR2Structure->op = Operator::NAND;
            Node *OR2StructureLeft = new Node("", OR2Structure);
            Node *OR2StructureRight = new Node("", OR2Structure);
            OR2StructureLeft->op = Operator::NOT;
            OR2StructureRight->op = Operator::NOT;
            Node *OR2StructureLeftLeft = new Node("", OR2StructureLeft);
            Node *OR2StructureRightLeft = new Node("", OR2StructureRight);
            OR2StructureLeftLeft->op = Operator::INPUT;
            OR2StructureRightLeft->op = Operator::INPUT;
            OR2StructureLeft->left = OR2StructureLeftLeft;
            OR2StructureRight->left = OR2StructureRightLeft;
            OR2Structure->left = OR2StructureLeft;
            OR2Structure->right = OR2StructureRight;

            AOI21Structure->op = Operator::NOT;
            Node *AOI21StructureLeft = new Node("", AOI21Structure);
            AOI21StructureLeft->op = Operator::NAND;
            Node *AOI21StructureLeftLeft = new Node("", AOI21StructureLeft);
            Node *AOI21StructureLeftRight = new Node("", AOI21StructureLeft);
            AOI21StructureLeftLeft->op = Operator::NAND;
            Node *AOI21StructureLeftLeftLeft = new Node("", AOI21StructureLeftLeft);
            Node *AOI21StructureLeftLeftRight = new Node("", AOI21StructureLeftLeft);
            AOI21StructureLeftLeftLeft->op = Operator::INPUT;
            AOI21StructureLeftLeftRight->op = Operator::INPUT;
            AOI21StructureLeftLeft->left = AOI21StructureLeftLeftLeft;
            AOI21StructureLeftLeft->right = AOI21StructureLeftLeftRight;
            AOI21StructureLeftRight->op = Operator::NOT;
            Node *AOI21StructureLeftRightLeft = new Node("", AOI21StructureLeftRight);
            AOI21StructureLeftRightLeft->op = Operator::INPUT;
            AOI21StructureLeftRight->left = AOI21StructureLeftRightLeft;
            AOI21StructureLeft->left = AOI21StructureLeftLeft;
            AOI21StructureLeft->right = AOI21StructureLeftRight;
            AOI21Structure->left = AOI21StructureLeft;

            AOI22Structure->op = Operator::NOT;
            Node *AOI22StructureLeft = new Node("", AOI22Structure);
            AOI22StructureLeft->op = Operator::NAND;
            Node *AOI22StructureLeftLeft = new Node("", AOI22StructureLeft);
            Node *AOI22StructureLeftRight = new Node("", AOI22StructureLeft);
            AOI22StructureLeftLeft->op = Operator::NAND;
            Node *AOI22StructureLeftLeftLeft = new Node("", AOI22StructureLeftLeft);
            Node *AOI22StructureLeftLeftRight = new Node("", AOI22StructureLeftLeft);
            AOI22StructureLeftLeftLeft->op = Operator::INPUT;
            AOI22StructureLeftLeftRight->op = Operator::INPUT;
            AOI22StructureLeftLeft->left = AOI22StructureLeftLeftLeft;
            AOI22StructureLeftLeft->right = AOI22StructureLeftLeftRight;
            AOI22StructureLeftRight->op = Operator::NAND;
            Node *AOI22StructureLeftRightLeft = new Node("", AOI22StructureLeftRight);
            Node *AOI22StructureLeftRightRight = new Node("", AOI22StructureLeftRight);
            AOI22StructureLeftRightLeft->op = Operator::INPUT;
            AOI22StructureLeftRightRight->op = Operator::INPUT;
            AOI22StructureLeftRight->left = AOI22StructureLeftRightLeft;
            AOI22StructureLeftRight->right = AOI22StructureLeftRightRight;
            AOI22StructureLeft->left = AOI22StructureLeftLeft;
            AOI22StructureLeft->right = AOI22StructureLeftRight;
            AOI22Structure->left = AOI22StructureLeft;
        }

        vector<string> findMatchingStructures(Node* topNode) {
            vector<string> matchingStructures;

            // Recursive lambda to traverse the DAG and match with template structures
            std::function<void(Node*, const string&)> traverse = [&](Node* node, const string& structureName) {
                if (!node) return;
                
                if (structureName == "NOT" && areStructurallyEquivalent(node, NOTStructure)) {
                    matchingStructures.push_back("NOT");
                } else if (structureName == "NAND2" && areStructurallyEquivalent(node, NAND2Structure)) {
                    matchingStructures.push_back("NAND2");
                } else if (structureName == "AND2" && areStructurallyEquivalent(node, AND2Structure)) {
                    matchingStructures.push_back("AND2");
                } else if (structureName == "NOR2" && areStructurallyEquivalent(node, NOR2Structure)) {
                    matchingStructures.push_back("NOR2");
                } else if (structureName == "OR2" && areStructurallyEquivalent(node, OR2Structure)) {
                    matchingStructures.push_back("OR2");
                } else if (structureName == "AOI21" && areStructurallyEquivalent(node, AOI21Structure)) {
                    matchingStructures.push_back("AOI21");
                } else if (structureName == "AOI22" && areStructurallyEquivalent(node, AOI22Structure)) {
                    matchingStructures.push_back("AOI22");
                }

                // Continue traversal
                traverse(node->left, structureName);
                traverse(node->right, structureName);
            };

            // Start the traversal from the top node
            traverse(topNode, "NOT");
            traverse(topNode, "NAND2");
            traverse(topNode, "AND2");
            traverse(topNode, "NOR2");
            traverse(topNode, "OR2");
            traverse(topNode, "AOI21");
            traverse(topNode, "AOI22");

            return matchingStructures;
        }
};