#include <iostream>
#include <vector>
#include <numeric> // For std::iota
#include <random>  // For std::mt19937, std::uniform_int_distribution
#include <ctime>   // For std::time
#include <cstdlib> // For std::atoi, std::exit
#include <queue>   // For std::queue (BFS)
#include <stdexcept> // For std::invalid_argument, std::out_of_range
#include <algorithm> // For std::shuffle

#include "hexpathfinder.h"

using namespace std;

//-----------------------------------------------------------------------------
// Disjoint Set Union (DSU) Data Structure
// Used for maze generation to detect cycles.
//-----------------------------------------------------------------------------
struct DSU {
    vector<uint32_t> parent;
    DSU(uint32_t n) {
        parent.resize(n);
        iota(parent.begin(), parent.end(), 0); // Fill with 0, 1, 2, ...
    }

    // Find the representative (root) of the set containing element i
    uint32_t find(uint32_t i) {
        if (parent[i] == i)
            return i;
        return parent[i] = find(parent[i]); // Path compression
    }

    // Unite the sets containing elements i and j
    void unite(uint32_t i, uint32_t j) {
        uint32_t root_i = find(i);
        uint32_t root_j = find(j);
        if (root_i != root_j) {
            parent[root_i] = root_j; // Make root_j the parent of root_i
        }
    }
};

//-----------------------------------------------------------------------------
// Wall Structure
// Represents a potential wall to be removed during generation.
// Stores the coordinates of *one* cell and the direction of the wall relative to that cell.
//-----------------------------------------------------------------------------
struct Wall {
    uint32_t r;          // Row of the cell
    uint32_t c;          // Column of the cell
    uint8_t direction; // Direction of the wall (e.g., WALL_DOWN, WALL_UP_RIGHT)

    // Overload == operator for potential use in Sampler if needed (e.g., checking duplicates)
    bool operator==(const Wall& other) const {
        return r == other.r && c == other.c && direction == other.direction;
    }
};


//-----------------------------------------------------------------------------
// Helper function: Get Neighbor Coordinates
// Calculates the coordinates (neighborR, neighborC) of the cell adjacent
// to (r, c) in the given wallDirection.
// Returns true if the neighbor is within the grid bounds (0 <= r < nR, 0 <= c < nC),
// false otherwise.
//-----------------------------------------------------------------------------
bool getNeighbor(uint32_t r, uint32_t c, uint8_t wallDirection, uint32_t nR, uint32_t nC, uint32_t &neighborR, uint32_t &neighborC) {
    int nr_int = static_cast<int>(r); // Use signed int for calculations
    int nc_int = static_cast<int>(c);
    int nR_int = static_cast<int>(nR);
    int nC_int = static_cast<int>(nC);

    int tempR = nr_int;
    int tempC = nc_int;

    // Calculate potential neighbor coordinates based on direction and column parity
    switch (wallDirection) {
        case WALL_UP:
            tempR--;
            break;
        case WALL_DOWN:
            tempR++;
            break;
        case WALL_UP_RIGHT:
            tempR = nr_int - 1 + (nc_int & 1); // r = r - 1 (even col), r (odd col)
            tempC++;
            break;
        case WALL_DOWN_RIGHT:
            tempR = nr_int + (nc_int & 1);     // r = r (even col), r + 1 (odd col)
            tempC++;
            break;
        case WALL_UP_LEFT:
            tempR = nr_int - 1 + (nc_int & 1); // r = r - 1 (even col), r (odd col)
            tempC--;
            break;
        case WALL_DOWN_LEFT:
            tempR = nr_int + (nc_int & 1);     // r = r (even col), r + 1 (odd col)
            tempC--;
            break;
        default:
            return false; // Invalid direction
    }

    // Check if the calculated neighbor coordinates are within the grid bounds
    if (tempR >= 0 && tempR < nR_int && tempC >= 0 && tempC < nC_int) {
        neighborR = static_cast<uint32_t>(tempR);
        neighborC = static_cast<uint32_t>(tempC);
        return true;
    } else {
        return false; // Neighbor is outside the grid
    }
}


//-----------------------------------------------------------------------------
// Maze Generation (Algorithm 1 from PDF)
// Uses DSU and randomized wall removal.
//-----------------------------------------------------------------------------
void generateMaze(uint8_t maze[][MAX_COLS], uint32_t nR, uint32_t nC, mt19937& rng) {
    // 1. Initialize maze with all walls present
    for (uint32_t r = 0; r < nR; ++r) {
        for (uint32_t c = 0; c < nC; ++c) {
            maze[r][c] = ALL_WALLS; // Set all wall bits
        }
    }

    // 2. Initialize Disjoint Set Union (DSU) structure
    uint32_t totalCells = nR * nC;
    DSU dsu(totalCells);

    // 3. Create a list of all *internal* walls to consider removing
    vector<Wall> internalWalls;
    internalWalls.reserve(totalCells * 3); // Approximate reservation

    for (uint32_t r = 0; r < nR; ++r) {
        for (uint32_t c = 0; c < nC; ++c) {
            uint32_t neighborR, neighborC;

            // Consider WALL_DOWN only if the cell below is valid
            if (getNeighbor(r, c, WALL_DOWN, nR, nC, neighborR, neighborC)) {
                 internalWalls.push_back({r, c, WALL_DOWN});
            }
            // Consider WALL_UP_RIGHT only if the neighbor is valid
             if (getNeighbor(r, c, WALL_UP_RIGHT, nR, nC, neighborR, neighborC)) {
                 internalWalls.push_back({r, c, WALL_UP_RIGHT});
            }
             // Consider WALL_DOWN_RIGHT only if the neighbor is valid
             if (getNeighbor(r, c, WALL_DOWN_RIGHT, nR, nC, neighborR, neighborC)) {
                 internalWalls.push_back({r, c, WALL_DOWN_RIGHT});
            }
            // Note: We only need to add walls in 3 directions from each cell
            // to cover all internal walls exactly once. Adding WALL_UP, WALL_UP_LEFT,
            // and WALL_DOWN_LEFT would be redundant.
        }
    }

    // 4. Shuffle the list of internal walls randomly
    shuffle(internalWalls.begin(), internalWalls.end(), rng);

    // 5. Remove walls until nR * nC - 1 walls have been removed (or all cells are connected)
    uint32_t wallsRemoved = 0;
    uint32_t targetWallsToRemove = totalCells - 1;

    for (const auto& wall : internalWalls) {
        if (wallsRemoved >= targetWallsToRemove) {
            break; // Stop once the maze is a spanning tree
        }

        uint32_t r1 = wall.r;
        uint32_t c1 = wall.c;
        uint8_t direction = wall.direction;
        uint32_t r2, c2;

        // Get the neighbor cell on the other side of the wall
        if (getNeighbor(r1, c1, direction, nR, nC, r2, c2)) {
            // Convert cell coordinates to DSU indices
            uint32_t cell1_idx = r1 * nC + c1;
            uint32_t cell2_idx = r2 * nC + c2;

            // Check if the cells are already connected using DSU
            if (dsu.find(cell1_idx) != dsu.find(cell2_idx)) {
                // If not connected, remove the wall and unite the sets
                uint8_t oppositeWall = getOppositeWall(direction);

                maze[r1][c1] &= ~direction;    // Remove wall from cell 1
                maze[r2][c2] &= ~oppositeWall; // Remove corresponding wall from cell 2

                dsu.unite(cell1_idx, cell2_idx); // Unite the sets in DSU
                wallsRemoved++;
            }
        }
    }

     if (wallsRemoved < targetWallsToRemove) {
        cerr << "Warning: Could not remove the target number of walls. Maze might not be fully connected." << endl;
    }
    // Optional: Implement Algorithm 2 here to remove additional walls if desired
}


//-----------------------------------------------------------------------------
// Maze Solving (Algorithm 3 from PDF)
// Uses Breadth-First Search (BFS) to find the shortest path.
//-----------------------------------------------------------------------------
void solveMazeBFS(uint8_t maze[][MAX_COLS], uint32_t nR, uint32_t nC) {
    // 1. Initialize count array and queue for BFS
    int count[MAX_ROWS][MAX_COLS]; // Stores distance from end cell
    queue<uint32_t> q;             // Stores cell indices (r * nC + c)

    for (uint32_t r = 0; r < nR; ++r) {
        for (uint32_t c = 0; c < nC; ++c) {
            count[r][c] = -1; // Initialize all counts to -1 (unvisited)
             maze[r][c] &= ~VISITED; // Clear any previous VISITED flags
        }
    }

    // 2. Start BFS from the end cell (bottom-right)
    uint32_t startR = 0;
    uint32_t startC = 0;
    uint32_t endR = nR - 1;
    uint32_t endC = nC - 1;

    if (endR >= nR || endC >= nC) {
         cerr << "Error: End cell coordinates are invalid." << endl;
         return;
    }


    uint32_t endCellIdx = endR * nC + endC;
    count[endR][endC] = 0; // Distance from end cell to itself is 0
    q.push(endCellIdx);

    // 3. Perform BFS
    while (!q.empty()) {
        uint32_t currentIdx = q.front();
        q.pop();

        uint32_t r = currentIdx / nC;
        uint32_t c = currentIdx % nC;

        // Explore neighbors
        uint8_t directions[] = {WALL_UP, WALL_DOWN, WALL_UP_LEFT, WALL_UP_RIGHT, WALL_DOWN_LEFT, WALL_DOWN_RIGHT};
        for (uint8_t dir : directions) {
            // Check if there is *no* wall in this direction
            if ((maze[r][c] & dir) == 0) {
                uint32_t neighborR, neighborC;
                // Get the valid neighbor coordinates
                if (getNeighbor(r, c, dir, nR, nC, neighborR, neighborC)) {
                    // Check if the neighbor hasn't been visited yet (count == -1)
                    if (count[neighborR][neighborC] == -1) {
                        count[neighborR][neighborC] = count[r][c] + 1; // Set distance
                        q.push(neighborR * nC + neighborC);           // Add neighbor to queue
                    }
                }
            }
        }
    }

    // 4. Trace the path back from the start cell (top-left) if reachable
    if (count[startR][startC] == -1) {
        cout << "No solution path found from start to end." << endl;
        return; // Start cell was not reached by BFS
    }

    uint32_t currentR = startR;
    uint32_t currentC = startC;
    maze[currentR][currentC] |= VISITED; // Mark start cell as visited

    while (count[currentR][currentC] != 0) { // While not back at the end cell
        bool foundNext = false;
        uint8_t directions[] = {WALL_UP, WALL_DOWN, WALL_UP_LEFT, WALL_UP_RIGHT, WALL_DOWN_LEFT, WALL_DOWN_RIGHT};
        for (uint8_t dir : directions) {
             // Check if there is *no* wall in this direction
             if ((maze[currentR][currentC] & dir) == 0) {
                uint32_t neighborR, neighborC;
                if (getNeighbor(currentR, currentC, dir, nR, nC, neighborR, neighborC)) {
                    // Check if this neighbor is the next step towards the end (count is one less)
                    if (count[neighborR][neighborC] == count[currentR][currentC] - 1) {
                        currentR = neighborR;
                        currentC = neighborC;
                        maze[currentR][currentC] |= VISITED; // Mark this cell as part of the path
                        foundNext = true;
                        break; // Move to the next step
                    }
                }
            }
        }
         if (!foundNext) {
             cerr << "Error: Could not trace path back from (" << currentR << "," << currentC << ") with count " << count[currentR][currentC] << endl;
             // This should not happen if BFS completed correctly and start was reachable
             return;
         }
    }
}


//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    // 1. Check and parse command-line arguments
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <num_rows> <num_cols>" << endl;
        return 1; // Indicate error
    }

    uint32_t nR, nC;
    try {
        int rows = stoi(argv[1]);
        int cols = stoi(argv[2]);

        if (rows <= 0 || rows > MAX_ROWS || cols <= 0 || cols > MAX_COLS) {
            throw out_of_range("Dimensions out of range.");
        }
        nR = static_cast<uint32_t>(rows);
        nC = static_cast<uint32_t>(cols);
    } catch (const invalid_argument& e) {
        cerr << "Error: Invalid number format for rows or columns." << endl;
        return 1;
    } catch (const out_of_range& e) {
        cerr << "Error: Rows must be between 1 and " << MAX_ROWS
             << ", Columns must be between 1 and " << MAX_COLS << "." << endl;
        return 1;
    }

    // 2. Seed the random number generator
    mt19937 rng(time(0)); // Mersenne Twister engine seeded with time

    // 3. Declare the maze array
    // Using static allocation since MAX_ROWS/MAX_COLS are constants
    // For very large mazes, dynamic allocation might be better.
    static uint8_t maze[MAX_ROWS][MAX_COLS];

    // 4. Generate the maze
    cout << "Generating " << nR << "x" << nC << " maze..." << endl;
    generateMaze(maze, nR, nC, rng);
    cout << "Maze generation complete." << endl;

    // 5. Solve the maze using BFS
    cout << "Solving maze using BFS..." << endl;
    solveMazeBFS(maze, nR, nC);
    cout << "Maze solving complete." << endl;

    // 6. Print the maze (generates maze.ps)
    cout << "Printing maze to maze.ps..." << endl;
    printMaze(maze, nR, nC);

    return 0; // Indicate success
}
