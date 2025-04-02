#ifndef HEXPATHFINDER_H
#define HEXPATHFINDER_H

#include <cstdint>
#include <vector> // Needed for potential helper structures/functions if added later

// --- Constants ---
const uint32_t MAX_ROWS = 50;
const uint32_t MAX_COLS = 50;

// Drawing constants (as provided)
const uint32_t DRAW_E = 6;       // Horizontal distance between center and vertical edge
const uint32_t DRAW_V = 5;       // Vertical distance between center and horizontal edge
const uint32_t DRAW_X_LEFT = 54; // Leftmost X coordinate for drawing
const uint32_t DRAW_Y_TOP = 708; // Topmost Y coordinate for drawing

// Macros to compute drawing coordinates (as provided)
#define computeX(c) (DRAW_X_LEFT + DRAW_E + (3 * DRAW_E * (c)) / 2)
#define computeY(r, c) (DRAW_Y_TOP - DRAW_V - 2 * DRAW_V * (r) - ((c)&1u) * DRAW_V)

// --- Cell Values (Bitmasks) ---
// Represents walls *present* in a cell
enum CellValues : uint8_t { // Use uint8_t explicitly
    WALL_UP = 0x01u,
    WALL_UP_RIGHT = 0x02u,
    WALL_DOWN_RIGHT = 0x04u,
    WALL_DOWN = 0x08u,
    WALL_DOWN_LEFT = 0x10u,
    WALL_UP_LEFT = 0x20u,
    ALL_WALLS = 0x3Fu, // Mask for all 6 walls
    VISITED = 0x40u,   // Flag for BFS path solution
    DEAD_END = 0x80u   // Flag for dead ends (optional, not used in final solution path marking)
};

// --- Function Declarations ---

// Provided drawing function (implementation in hexpathfinder_draw.cpp)
void printMaze(uint8_t maze[][MAX_COLS], uint32_t nR, uint32_t nC);

// --- Helper Function Declarations (Optional but Recommended) ---
// You might want to add helper functions here, e.g., for getting neighbors

// Function to get the opposite wall direction
inline uint8_t getOppositeWall(uint8_t wallDirection) {
    switch (wallDirection) {
    case WALL_UP: return WALL_DOWN;
    case WALL_UP_RIGHT: return WALL_DOWN_LEFT;
    case WALL_DOWN_RIGHT: return WALL_UP_LEFT;
    case WALL_DOWN: return WALL_UP;
    case WALL_DOWN_LEFT: return WALL_UP_RIGHT;
    case WALL_UP_LEFT: return WALL_DOWN_RIGHT;
    default: return 0; // Should not happen
    }
}

// Function to get neighbor coordinates (implement in main.cpp or a helper file)
bool getNeighbor(uint32_t r, uint32_t c, uint8_t wallDirection, uint32_t nR, uint32_t nC, uint32_t &neighborR, uint32_t &neighborC);


#endif // HEXPATHFINDER_H
