# Excursion 2: Technology Mapping

## Project Overview
This C++ project performs technology mapping to determine the minimum cost for constructing a circuit using a library of NAND/NOT gates. It utilizes a bottom-up dynamic programming approach.

## System Requirements
- C++11 or later
- A compiler like g++ or clang

## Installation
Clone the repository to your local machine and build the project using a C++ compiler.

```
g++ -std=c++11 main.cpp -o techmap
```

## Usage
Run the executable in the command line.

```
./techmap
```

Enter the name of the file containing the netlist when prompted. If no filename is entered, ```input1.net``` is used by default.

## Input File Format
The input file should be a netlist describing the circuit in the ``input/`` directory. The netlist file should contain exactly one OUTPUT expression declaration and at least one INPUT expression delcaration. All expressions in the netlist representing AND, OR, or NOT gates should be previously defined using the ```=``` token, followed by exactly one ```AND```, ```OR```, or ```NOT``` token and an appropriate number of inputs for that gate.

## Output
The program outputs to ```output.net```, which describes the netlist using the minimum cost combination of NAND/NOT gates from the technology library, and prints the minimum cost to the console. The “technology library” includes NOT, NAND2, AND2, NOR2, OR2, AO121 and AO122 functions with respective costs 2, 3, 4, 6, 4, 7, and 7.

## Components
+ ```main.cpp```: Program entry point. Manages file input and begins the mapping process.
+ ```node.h```: Defines Node structure and functions for constructing/managing the DAG.
+ ```dp.h```: Implements dynamic programming algorithm for minimum cost calculation.
+ ```dag.h```: Represents the provided netlist as a directed acyclic graph.

## Authors
Shane Ferrell

Devin Willis

Nickolas Arustamyan
