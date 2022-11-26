#ifndef DATABASE_H
#define DATABASE_H

#include <stdbool.h>

#define GRID_MAX    30
#define MM_MAX      5
#define NSEW        4

#define FRONT    0
#define LEFT    -1
#define RIGHT    1
#define BEHIND   2

// Enum to keep track of the direction
typedef enum DIRECTION
{
    NORTH = 0,
    EAST,
    SOUTH,
    WEST
} DIRECTION;


// Struct for Grid
// x = x axis coordinate, y = y axis coordinate
// north, south, east and west are General Directions
// By default, when the vehicle starts, it will be facing a hypothetical "north".
typedef struct Grid Grid;

struct Grid
{
    // coordinate of the grid
    short x;
    short y;

    // addresses of the neighbouringGrids in array
    // slot 0 = North grid, slot 1 = East grid, slot 2 = South grid, slot 3 = West grid
    Grid *neighbouringGrids[NSEW];

    // boolean to track if the grid had been visited
    bool visited;

};


typedef struct
{
    // Initialise an array of Grid struct
    Grid gridList[GRID_MAX];
    Grid *MMList[MM_MAX];
    
    Grid *Path[GRID_MAX];

    bool isPathfinding;
    DIRECTION isFacing;
    
    // Save whole gridList as memory location
    Grid *currentGrid; // init Current = first element of gridlist array
    
    int gridCounter;

} Database;

int WrapNSEW(int result)
{
    while (result < 0)
    {
        result += NSEW;
    }
    if (result >= NSEW)
    {
        result %= NSEW;
    }
    return result;
}

#endif