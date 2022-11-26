#ifndef DATABASE_H
#define DATABASE_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/timer.h"


#include "string.h"

// Ultrasonic pins
#define LEFT_TRIG 10 
#define LEFT_ECHO 11
#define CENTER_TRIG 14
#define CENTER_ECHO 15
#define RIGHT_TRIG 16
#define RIGHT_ECHO 17
#define DIST_FROM_WALL 10


// Maping defines
#define GRID_MAX    30 // max number of grids
#define MM_MAX      5 // max number of multi move list elements
#define NSEW        4 // max number of directions

// offset for directions
// e.g NORTH is 0 in DIRECTION enum, by inrementing it by +1, 
// the value will now be 1, which now represents EAST
// so by adding 1, we can offset the direction to the RIGHT
// and subtracting 1 will shift to the LEFT
#define FRONT    0
#define LEFT    -1
#define RIGHT    1
#define BEHIND   2

#define UPDATE_COMMS_INTERVAL 500

#define GRID_NOTCH 20

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
    // list of grids
    Grid gridList[GRID_MAX];
    
    // This is a list of grids that we want to return to.
    // Grids are added to this list when there are 2 or more posible paths to take,
    // and they are not the grid that is prioritized by the maping algorithim.
    // so we keep track of them to retun here later.
    Grid *MMList[MM_MAX];
    
    // path of grids that the path finding algo will generate
    Grid *Path[GRID_MAX];
    // pathfinding mode toggle
    bool isPathfinding;

    // current direction of the car
    DIRECTION isFacing;
    
    // current grid the car is on
    Grid *currentGrid; 
    
    // counter that will increment every time a new grid is discovered
    int gridCounter;

    char *barcodeResult;

    int wheelSpeedL;
    int wheelSpeedR;

} Database;

// variables
Database *database;
int motor_actionCode = 0;

/**
 * @brief Wrap integer to be within 0 to 3, mainly for accessing grid's neighbouringGrids.
 * a use case is when direction is NORTH (value 0) and i want to turn to the left which is
 * WEST (value 3), i will perform a -1, get result of -1, and will have to wrap result
 * back up to 3 in order for it to represent WEST.
 */
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