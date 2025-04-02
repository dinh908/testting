//
// Created by bob on 11/23/18.
// (Or adapted from user provided code)
// Contains the implementation for drawing the maze to a PostScript file.
//

#include <fstream>
#include <iostream>
#include "hexpathfinder.h"

using namespace std;

// Helper function to draw a line in PostScript format
void drawLine(ofstream &outFile, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2) {
    outFile << "newpath "
            << x1 << ' ' << y1 << " moveto "
            << x2 << ' ' << y2 << " lineto stroke\n";
}

// Main function to draw the maze structure and optionally the solution path
void drawMaze(ofstream &outFile, uint8_t maze[][MAX_COLS], uint32_t nR, uint32_t nC,
              bool drawSolution, bool drawDeadEnds) { // drawDeadEnds is unused based on printMaze call
    uint32_t
        r, c,
        r2, c2,
        x, y,
        x2, y2;

    outFile << "0.25 setlinewidth\n"; // Set line width for walls

    // --- Draw Internal Walls ---
    // Iterate through each cell and draw walls that are present
    for (r = 0; r < nR; r++) {
        for (c = 0; c < nC; c++) {
            x = computeX(c);
            y = computeY(r, c);

            // Draw walls based on flags set in the maze array
            // Only draw UP_RIGHT, DOWN_RIGHT, and DOWN to avoid drawing walls twice
            if (maze[r][c] & WALL_UP_RIGHT)
                drawLine(outFile, x + DRAW_E / 2, y + DRAW_V, x + DRAW_E, y);
            if (maze[r][c] & WALL_DOWN_RIGHT)
                drawLine(outFile, x + DRAW_E, y, x + DRAW_E / 2, y - DRAW_V);
            if (maze[r][c] & WALL_DOWN)
                drawLine(outFile, x + DRAW_E / 2, y - DRAW_V, x - DRAW_E / 2, y - DRAW_V);
        }
    }

    // --- Draw Exterior Walls ---
    // These walls are always present unless explicitly removed at borders (which isn't typical for this maze type)

    // Draw the left walls (UP_LEFT and DOWN_LEFT) for column 0
    for (r = 0; r < nR; r++) {
        x = computeX(0);
        y = computeY(r, 0u); // 0u specifies unsigned int literal
        drawLine(outFile, x - DRAW_E / 2, y + DRAW_V, x - DRAW_E, y);       // Up-Left
        drawLine(outFile, x - DRAW_E, y, x - DRAW_E / 2, y - DRAW_V); // Down-Left
    }

    // Draw the top walls (UP) for row 0
    for (c = 0; c < nC; c++) {
        x = computeX(c);
        y = computeY(0, c);
        drawLine(outFile, x - DRAW_E / 2, y + DRAW_V, x + DRAW_E / 2, y + DRAW_V); // Up
    }

    // Draw the top-left walls (UP_LEFT) for row 0 in even columns (c > 0)
    // These connect the top row cells diagonally
    for (c = 2; c < nC; c += 2) {
        x = computeX(c);
        y = computeY(0, c);
        drawLine(outFile, x - DRAW_E / 2, y + DRAW_V, x - DRAW_E, y); // Up-Left
    }

     // Draw the bottom-left walls (DOWN_LEFT) for the bottom row (nR-1) in odd columns
    // These connect the bottom row cells diagonally
    for (c = 1; c < nC; c += 2) {
        x = computeX(c);
        y = computeY(nR - 1, c);
        drawLine(outFile, x - DRAW_E / 2, y - DRAW_V, x - DRAW_E, y); // Down-Left
    }


    // --- Draw Solution Path (if requested) ---
    if (drawSolution) {
        // Set color (blue) and line width for the solution path
        outFile << "0 0 1 setrgbcolor gsave currentlinewidth 5 mul setlinewidth "
                   " 1 setlinecap\n"; // Blue, thicker line, rounded caps

        int count[MAX_ROWS][MAX_COLS]; // Need the count array from BFS to trace path
        // NOTE: This drawMaze function doesn't have access to the 'count' array
        // The logic below assumes 'VISITED' flag correctly marks the path cells.

        for (r = 0; r < nR; r++) {
            for (c = 0; c < nC; c++) {
                // Check if the cell is part of the solution path (marked as VISITED but not a DEAD_END)
                // The original PDF implies VISITED marks the final path.
                if ((maze[r][c] & VISITED) != 0) {
                     x = computeX(c);
                     y = computeY(r, c);

                    // Check each neighbor. If the neighbor is also on the path and there's no wall, draw a line segment.
                    uint32_t neighborR, neighborC;
                    uint8_t directions[] = {WALL_UP, WALL_DOWN, WALL_UP_LEFT, WALL_UP_RIGHT, WALL_DOWN_LEFT, WALL_DOWN_RIGHT};

                    for(uint8_t dir : directions) {
                        // Check if there is *no* wall in this direction for the current cell
                        if ((maze[r][c] & dir) == 0) {
                             // Get the coordinates of the neighbor in that direction
                            if (getNeighbor(r, c, dir, nR, nC, neighborR, neighborC)) {
                                // Check if the neighbor is also part of the visited path
                                if ((maze[neighborR][neighborC] & VISITED) != 0) {
                                    // Calculate neighbor's center coordinates
                                    x2 = computeX(neighborC);
                                    y2 = computeY(neighborR, neighborC);

                                    // Draw line segment between centers (or midpoints for smoother look)
                                    // Draw only half the line to avoid drawing each segment twice?
                                    // Let's draw the full line for simplicity, PostScript might handle overlaps.
                                    // Only draw if neighbor has higher index to draw each segment once
                                    if (neighborR * nC + neighborC > r * nC + c) {
                                         drawLine(outFile, x, y, x2, y2);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        outFile << "grestore\n"; // Restore graphics state (color, line width)
    }
     // --- Draw Dead Ends (if requested) ---
     // This section was commented out in the original printMaze, keeping it commented.
    /*
    if (drawDeadEnds) {
        outFile << "1 0 0 setrgbcolor\n"; // Red color for dead ends
        for (r = 0; r < nR; r++) {
            for (c = 0; c < nC; c++) {
                x = computeX(c);
                y = computeY(r, c);
                if ((maze[r][c] & DEAD_END) != 0) {
                    // Logic to draw lines indicating dead ends (e.g., short lines into the dead end passage)
                    // This requires checking which passage is open from the dead end cell.
                    uint8_t directions[] = {WALL_UP, WALL_DOWN, WALL_UP_LEFT, WALL_UP_RIGHT, WALL_DOWN_LEFT, WALL_DOWN_RIGHT};
                     for(uint8_t dir : directions) {
                        if ((maze[r][c] & dir) == 0) { // If there's no wall (it's an open passage)
                            if (getNeighbor(r, c, dir, nR, nC, r2, c2)) {
                                 x2 = computeX(c2);
                                 y2 = computeY(r2, c2);
                                 // Draw a short line from center towards the neighbor
                                 drawLine(outFile, x, y, (x + x2) / 2, (y + y2) / 2);
                                 break; // Usually a dead end has only one exit
                            }
                        }
                     }
                }
            }
        }
         outFile << "0 0 0 setrgbcolor\n"; // Reset color to black
    }
    */

}


// Function to create the PostScript file and call drawMaze
void printMaze(uint8_t maze[][MAX_COLS], uint32_t nR, uint32_t nC) {
    ofstream outFile;

    outFile.open("maze.ps"); // Open the output file
    if (!outFile) {
        cerr << "Error: cannot open maze.ps for writing." << endl; // Use cerr for errors
        return;
    }

    // --- Page 1: Maze Only ---
    outFile << "%!PS-Adobe-2.0\n\n%%Pages: 2\n%%Page: 1 1\n"; // PS Header

    // Title for page 1
    outFile << "/Arial findfont 20 scalefont setfont\n"
            << "54 730 moveto (Random Maze - " << nR << "x" << nC << ") show\n";

    // Draw the maze without the solution
    drawMaze(outFile, maze, nR, nC, false, false);

    outFile << "showpage\n"; // End page 1

    // --- Page 2: Maze With Solution ---
    outFile << "%%Page: 2 2\n"; // Header for page 2

    // Title for page 2
    outFile << "/Arial findfont 20 scalefont setfont\n"
            << "54 730 moveto (Random Maze With Solution - " << nR << "x" << nC << ") show\n";

    // Draw the maze *with* the solution path highlighted
    drawMaze(outFile, maze, nR, nC, true, false); // drawSolution = true

    outFile << "showpage\n"; // End page 2

    // --- Optional Page 3 (Commented out as in original) ---
    /*
    outFile << "%%Page: 3 3\n";
    outFile << "/Arial findfont 20 scalefont setfont\n"
            << "54 730 moveto (Random Maze With Solution and Dead Ends) show\n";
    drawMaze(outFile, maze, nR, nC, true, true); // drawSolution = true, drawDeadEnds = true
    outFile << "showpage\n";
    */

    outFile.close(); // Close the file
    cout << "Maze written to maze.ps" << endl; // Confirmation message
}
