# Square-1 Two-Phase Solver in C

This project is a relatively optimized solver for the **Square-1** puzzle, developed in C as part of my TIPE (Scientific Research Project) during my CPGE MPI (Mathematics, Physics, and Computer Science) intensive preparatory program. 

It explores the combinatorial state space of the puzzle to find a solution in **fewer than 16 moves** (based on a sample of over 100 000 scrambles).

## Performance
* 100,000 random scrambles tested
* 100,000 / 100,000 solved in <16 moves
* ~2 min 30 total runtime for 100,000 scrambles
* ~667 scrambles/s
* Single-threaded
* Can probably run more scrambles, thus the random scramble generator should be upgraded (it does not ensure that every scramble is unique)

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

This project was inspired by the work of Jaap Scherphuis who studied the mathematical structure of multiple variants of the rubik's cube, one being the square-1.

This program explains its use by launching it without any arguments :
Usage:
  ./square1_solver <position>          solve a position
  ./square1_solver -r [N] [seed]       test N random scrambles (défaut: 100), the seed format is a normal integer

Position : 16 or 17 caracters (A-H corners, 1-8 edges)
Example solved state : A1B2C3D45E6F7G8H-
Example parity : A1B2C3D46E5F7G8H/ (/ or - represents the orientation of the middle layer)
