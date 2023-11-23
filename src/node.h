#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>

using namespace std;

// forward declaration
class Node;

// operator enum declaration
enum class Operator { AND, OR, NOT, NAND, INPUT };

static int numberOfAndOrNotNodes = 0;
static unordered_map<string, Node*> constructedNodes;

class Node
{
    string name;
    unsigned int id;
    Operator op;
    Node* parent;
    Node *left, *right;

    Node(string name, Node* parent)
    {
        this->name = name;
        this->id = numberOfAndOrNotNodes++;
        this->parent = parent;
    }

    // construct root from OUTPUT line
    static Node* constructRoot(vector<string>& netlistLines)
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

                    Node* root = new Node(expr, nullptr);
                    constructedNodes[expr] = root;
                    return root;
                }
            }
        }
        throw invalid_argument("Netlist file does not contain OUTPUT expression\n");
    }

    static Node* constructDAGFromRoot(Node* root, vector<string>& netlistLines)
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
                            Node* left = new Node(expr1, root);
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
                            Node* left = new Node(expr1, root);
                            constructedNodes[expr1] = left;
                            root->left = constructDAGFromRoot(left, netlistLines);
                        }
                        else
                            root->left = constructedNodes[expr1];

                        if (constructedNodes.find(expr2) == constructedNodes.end())
                        {
                            Node* right = new Node(expr2, root);
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
    static Node* constructAndOrNotDAG(string filePath)
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

        Node* root = constructRoot(netlistLines);
        return constructDAGFromRoot(root, netlistLines);
    }

    static Node* constructNandNotDAG(Node* andOrNotTreeRoot)
    {

    }
};

