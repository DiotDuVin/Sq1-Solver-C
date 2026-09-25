# Square-1 Two-Phase Solver in C

This project is a relatively optimized solver for the **Square-1** puzzle, developed in C as part of my TIPE (Scientific Research Project) during my CPGE MPI (Mathematics, Physics, and Computer Science) intensive preparatory program. 

It explores the combinatorial state space of the puzzle to find an optimal solution in **fewer than 16 moves** (based on a sample of over 100 000 scrambles).

## Algorithmic Features
* **State Space Modeling:** Modeled the complex geometry and shape-shifting permutations of the Square-1 puzzle.
* **Two-Phase Algorithm:** Drastically reduced combinatorial complexity by splitting the resolution into two distinct phases using transition and pruning tables.
* **Graph Traversal & Optimization:** Implemented highly optimized graph traversal algorithms (BFS/IDA*) designed for large state spaces.
* **Memory Management:** Designed custom dynamic data structures with a strict focus on memory footprint optimization to prevent RAM explosion during state exploration.

## Tech Stack & Tools
* **Language:** C (Core logic, memory management, pointers)
* **Compiler:** GCC (with heavy optimization flags)
* **Environment:** Linux / Bash

## How to Run the Project
1. Clone this repository to your local machine.
2. Compile the source code using GCC with optimization enabled:
   ```bash
   gcc sq1_solver.c -o square1_solver
   ```
3. Run the executable:
   ```bash
   ./square1_solver
   ```
