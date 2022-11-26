// Team Mapping and Navigation

// switch pathfining algo with the folowing define
// define "SHORTEST_PATH" to run the shortest path algo
// remove the define to run the first path found algo
#define SHORTEST_PATH

#ifdef SHORTEST_PATH
#include "shortestPath.c"
#else
#include "firstPathFound.c"
#endif

#include <stdio.h>
#include "database.h"
#include "ultrasonic.c"

// Ultrasonic pins
#define U_TRIGGER_PIN 2
#define U_ECHO_PIN 3

typedef enum FINITE_STATE
{
    FS_CHECKING_GRID = 0,
    FS_GOING_STRAIGHT,
    FS_TURN_RIGHT,
    FS_TURN_LEFT,
    FS_TURN_AROUND,
    FS_PATHFINDING,
    FS_FINISH_MAPPING

} FINITE_STATE;

const char *direction_str[NSEW] = {"North", "East", "South", "West"};

// Function Prototypes

void PicoStartUpTest();
void CheckPosibleMoves(int L, int R, int F, Database *database);
void LinkCurrentGrid(int facingOffset, Database *database);
Grid *GetGrid(int nextX, int nextY, Database *database);
Grid *GetNextAvailableGrid(int nextX, int nextY, Database *database);
void SetCoordinate(int *x, int *y, DIRECTION facing);
int FindTurningIndex(Grid *grid, Database *database);
void UpdateMMList(int facingOffset, Database *database);
void RemoveFromMMList(Grid *MMgrid, Database *database);

unsigned char mygetchar() { 
     int c; 
     while ( (c = getchar_timeout_us(0)) < 0);  
     return (unsigned char)c; 
};

int main()
{
    // Init all standard I/O
    stdio_init_all();

    PicoStartUpTest();

    // ultrasonic init
    init_pins(U_TRIGGER_PIN, U_ECHO_PIN);

    // FSM current state
    FINITE_STATE curState = FS_CHECKING_GRID;
    // flag to toggle when finish mapping
    bool finishMapping = false;

    // shared database for all modules to use
    Database database;
    memset(&database, 0, sizeof(Database));

    InitAlgorithim(&database);

    // set the current grid to slot 0 of the grid list
    database.currentGrid = &(database.gridList[0]);
    database.currentGrid->visited = true;
    
    mygetchar();
    
    // Main loop, will keep running
    while (1)
    {
        // Final State Machine (FSM)
        switch (curState)
        {

        // Moving forward or just moving around?
        case FS_GOING_STRAIGHT:
        {
            printf("\n<GOING_STRAIGHT State>\n");
            unsigned char userInput = 0;
            do
            {
                printf("Finish moving straight? y/n: ");
                userInput = mygetchar();
                printf("%c\n", userInput);
            } while (userInput != 'y');

            // update current grid to next grid based on the direction of car
            database.currentGrid = database.currentGrid->neighbouringGrids[database.isFacing];
            database.currentGrid->visited = true;
            curState = FS_CHECKING_GRID;
        }
        break;

        case FS_TURN_LEFT:
        {
            printf("\n<TURN_LEFT State>\n");
            database.isFacing = WrapNSEW(database.isFacing + LEFT);

            unsigned char userInput = 0;
            do
            {
                printf("Finish turning left? y/n: ");
                userInput = mygetchar();
                printf("%c\n", userInput);
            } while (userInput != 'y');

            curState = FS_GOING_STRAIGHT;
        }
        break;

        case FS_TURN_RIGHT:
        {
            printf("\n<TURN_RIGHT State>\n");
            database.isFacing = WrapNSEW(database.isFacing + RIGHT);

            unsigned char userInput = 0;
            do
            {
                printf("Finish turning right? y/n: ");
                userInput = mygetchar();
                printf("%c\n", userInput);
            } while (userInput != 'y');

            curState = FS_GOING_STRAIGHT;
        }
        break;

        case FS_TURN_AROUND:
        {
            printf("\n<TURN_AROUND State>\n");
            database.isFacing = WrapNSEW(database.isFacing + BEHIND);

            unsigned char userInput = 0;
            do
            {
                printf("Finish 180 turn? y/n: ");
                userInput = mygetchar();
                printf("%c\n", userInput);
            } while (userInput != 'y');

            if (database.isPathfinding == true)
                curState = FS_GOING_STRAIGHT;
            else
                curState = FS_CHECKING_GRID;
        }
        break;

        case FS_CHECKING_GRID:
        {
            printf("\n<CHECKING_GRID State>\n");
            
            printf("\nCurrent gridList array slot: %d\n", database.gridCounter);
            printf("Current position x: %d, y: %d\n", database.currentGrid->x, database.currentGrid->y);
            printf("Currently facing: %s\n", direction_str[database.isFacing]);

            if (database.isPathfinding)
            {
                curState = FS_PATHFINDING;
                continue;
            }
            else if (finishMapping)
            {
                curState = FS_FINISH_MAPPING;
                continue;
            }

            // Get all 3 Ultrasonic's Current Distance (L,R,F)

            // Simulate Ultrasonic Sensors

            unsigned char userInputF = 0, userInputL = 0, userInputR = 0;
            do
            {
                printf("Can move %s? y/n: ", direction_str[database.isFacing]);
                userInputF = mygetchar();
                printf("%c\n", userInputF);
            } while (userInputF != 'y' && userInputF != 'n');

            do
            {
                printf("Can move %s? y/n: ", direction_str[WrapNSEW(database.isFacing + LEFT)]);
                userInputL = mygetchar();
                printf("%c\n", userInputL);
            } while (userInputL != 'y' && userInputL != 'n');

            do
            {
                printf("Can move %s? y/n: ", direction_str[WrapNSEW(database.isFacing + RIGHT)]);
                userInputR = mygetchar();
                printf("%c\n", userInputR);
            } while (userInputR != 'y' && userInputR != 'n');

            CheckPosibleMoves(
                userInputF == 'y' ? 200 : 0,
                userInputL == 'y' ? 200 : 0,
                userInputR == 'y' ? 200 : 0,
                &database);


            // check which direction is possible. Check Right, Left then Front.
            // check current grid's right side grid (isfacing+1) is not NULL (has a grid address) and that right side grid is not visited
            if (database.currentGrid->neighbouringGrids[WrapNSEW(database.isFacing + RIGHT)] != NULL && !database.currentGrid->neighbouringGrids[WrapNSEW(database.isFacing + RIGHT)]->visited)
            {
                UpdateMMList(RIGHT, &database);
                curState = FS_TURN_RIGHT;
            }
            else if (database.currentGrid->neighbouringGrids[WrapNSEW(database.isFacing + LEFT)] != NULL && !database.currentGrid->neighbouringGrids[WrapNSEW(database.isFacing + LEFT)]->visited)
            {
                UpdateMMList(LEFT, &database);
                curState = FS_TURN_LEFT;
            }
            else if (database.currentGrid->neighbouringGrids[(database.isFacing)] != NULL && !database.currentGrid->neighbouringGrids[(database.isFacing)]->visited)
            {
                curState = FS_GOING_STRAIGHT;
            }
            else // Cannot move forward. Left, Right and Front no possible move.
            {
                if (database.currentGrid->neighbouringGrids[WrapNSEW(database.isFacing + BEHIND)] == NULL)
                {
                    curState = FS_TURN_AROUND;
                    continue;
                }

                Grid *toMoveto = NULL;
                for (int i = 0; i < MM_MAX; i++)
                {
                    if (database.MMList[i] != NULL && database.MMList[i] != database.currentGrid)
                    {
                        toMoveto = database.MMList[i];
                        break;
                    }
                }

                if (toMoveto == NULL)
                {
                    printf("\nMM List is empty. Completed Mapping.\n");
                    finishMapping = true;
                    continue;
                }

                // When neighbors are already visited, start Dijkstra to navigate Multi-Move
                // Currently is only recursive path finding.
                #ifdef SHORTEST_PATH
                recurToDest(database.currentGrid, toMoveto, database.currentGrid, 0);
                #else
                recurToDest(database.currentGrid, toMoveto, database.currentGrid);
                #endif

            }
        }
        break;

        case FS_PATHFINDING:
        {
            printf("\n<PATHFINDING State>\n");

            static int i = 1;
            // traverse the path array
            if (database.Path[i] == NULL)
            {
                // We have travelled at the end of the path.
                // Move to MultiMove
                RemoveFromMMList(database.Path[i - 1], &database);
                database.isPathfinding = false;
                memset(&database.Path, 0, sizeof(database.Path));
                i = 1;

                curState = FS_CHECKING_GRID;
            }
            else
            {
                printf("\nRemainding path to take: ");
                for(int j = i; j < GRID_MAX; ++j)
                {   
                    if(database.Path[j] == NULL)
                        break;
                    printf("[x:%d, y:%d]->", database.Path[j]->x, database.Path[j]->y);
                }
                printf("DONE\n");

                switch (FindTurningIndex(database.Path[i++], &database))
                {
                case FRONT:
                    // move forward
                    curState = FS_GOING_STRAIGHT;
                    break;
                case RIGHT:
                    // Turn Right
                    curState = FS_TURN_RIGHT;
                    break;
                case LEFT:
                    curState = FS_TURN_LEFT;
                    break;
                    // move left, then forward
                case BEHIND:
                    curState = FS_TURN_AROUND;
                    break;
                    // 180 back
                }
            }
        }
        break;

        case FS_FINISH_MAPPING:
        {
            printf("\n<FINISH_MAPPING State>\n");
            printf("Enter navigation coordinates\n");

            int pathFindingX = 1, pathFindingY = 1;
            unsigned char userInput = 0;

            printf("Enter coordinates for X: ");
            userInput = mygetchar();
            if(userInput == '-')
            {
                printf("%c", userInput);
                pathFindingX = -1;
                userInput = mygetchar();
            }
            printf("%c", userInput);
            pathFindingX *= (userInput - '0');

            printf("\nEnter coordinates for Y: ");
            userInput = mygetchar();
            if(userInput == '-')
            {
                printf("%c", userInput);
                userInput = mygetchar();
                pathFindingY = -1;
            }
            printf("%c", userInput);
            pathFindingY *= (userInput - '0');

            // Start Path Finding
            Grid *pFinder = GetGrid(pathFindingX, pathFindingY, &database);
            if (pFinder != NULL && database.currentGrid != pFinder)
            {
                #ifdef SHORTEST_PATH
                recurToDest(database.currentGrid, pFinder, database.currentGrid, 0);
                #else
                recurToDest(database.currentGrid, pFinder, database.currentGrid);
                #endif

                curState = FS_CHECKING_GRID;
            }
            else
            {
                printf("\nCannot move to location\n");
            }
        }
        break;

        }
    }

    /*
    // Ultrasonic Sensor pulse
    uint64_t pulseTime = sendPulse(U_TRIGGER_PIN, U_ECHO_PIN);
    uint64_t pulseLength = (pulseTime / 2) / 29.1;
    printf("Pulse length: %llu\n", pulseLength);
    sleep_ms(3000);
    */
}

void PicoStartUpTest()
{
    // LED Light up and print to check if Pico starts up.
    // Sleep first to have time to connect into PUTTY.
    sleep_ms(5000);
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    sleep_ms(1000); // temporary
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    printf("SYSTEM ON, PRESS ANY KEY TO CONTINUE\n");
}

void CheckPosibleMoves(int F, int L, int R, Database *database)
{
    if (F > 100)
    {
        LinkCurrentGrid(FRONT, database);
    }
    if (L > 100)
    {
        LinkCurrentGrid(LEFT, database);
    }
    if (R > 100)
    {
        LinkCurrentGrid(RIGHT, database);
    }
}

void LinkCurrentGrid(int facingOffset, Database *database)
{
    // Get current car facing direction and shift it to direction of "empty" grid
    // Modulus 4(NSEW) is to determine new direction (enum roll over).
    DIRECTION tempFacing = WrapNSEW(database->isFacing + facingOffset);

    if (database->currentGrid->neighbouringGrids[tempFacing] != NULL)
    {
        return;
    }

    // Get position of the current grid
    int tempX = database->currentGrid->x;
    int tempY = database->currentGrid->y;

    // Offset the position based on which ever direction of the "empty" grid
    SetCoordinate(&tempX, &tempY, tempFacing);

    // GRID CHECKER
    // Assuming that there is only a maximum of 5 in the MM List.
    for (int i = 0; i < MM_MAX; i++)
    {
        // Check if this current grid is part of the MM list.
        if (database->MMList[i] != NULL && database->MMList[i]->x == tempX && database->MMList[i]->y == tempY)
        {
            // The grid has already been created as it is a MM grid
            // Put neighbour grid's address to current grid
            database->currentGrid->neighbouringGrids[tempFacing] = database->MMList[i];
            database->MMList[i]->neighbouringGrids[WrapNSEW(tempFacing + BEHIND)] = database->currentGrid;

            database->MMList[i] = 0;

            return;
        }
    }

    // Change counter to the next neighbouring grid
    Grid *nextGrid = GetNextAvailableGrid(tempX, tempY, database);
    // Put neighbour grid's address to current grid
    database->currentGrid->neighbouringGrids[tempFacing] = nextGrid;
    // Put the neighbouring grid to the current grid
    nextGrid->neighbouringGrids[WrapNSEW(tempFacing + BEHIND)] = database->currentGrid;

    // Set the x and y of the neighbouring grid
    nextGrid->x = tempX;
    nextGrid->y = tempY;
}

void SetCoordinate(int *x, int *y, DIRECTION facing)
{
    if (facing == NORTH)
    {
        (*y) += 1;
    }
    else if (facing == SOUTH)
    {
        (*y) -= 1;
    }
    else if (facing == EAST)
    {
        (*x) += 1;
    }
    else if (facing == WEST)
    {
        (*x) -= 1;
    }
}

Grid *GetGrid(int nextX, int nextY, Database *database)
{
    for (int i = 0; i <= database->gridCounter; ++i)
    {
        if (nextX == database->gridList[i].x && nextY == database->gridList[i].y)
            return &database->gridList[i];
    }
    return NULL;
}

Grid *GetNextAvailableGrid(int nextX, int nextY, Database *database)
{
    for (int i = 0; i <= database->gridCounter; ++i)
    {
        if (nextX == database->gridList[i].x && nextY == database->gridList[i].y)
            return &database->gridList[i];
    }

    database->gridCounter += 1;

    return &database->gridList[database->gridCounter];
}

int FindTurningIndex(Grid *grid, Database *database)
{
    int turningIndex = 0;

    if (grid->x < database->currentGrid->x)
        turningIndex = (WEST - database->isFacing);
    else if (grid->x > database->currentGrid->x)
        turningIndex = (EAST - database->isFacing);
    else if (grid->y < database->currentGrid->y)
        turningIndex = (SOUTH - database->isFacing);
    else
        turningIndex = (NORTH - database->isFacing);

    if (turningIndex == 3)
        turningIndex = -1;
    else if (turningIndex == -2)
        turningIndex = 2;
    else if (turningIndex == -3)
        turningIndex = 1;

    return turningIndex;
}

void UpdateMMList(int facingOffset, Database *database)
{
    for (int i = 0; i < NSEW; i++)
    {
        // if there is a grid other then right side of current
        if (database->currentGrid->neighbouringGrids[i] != NULL
        && database->currentGrid->neighbouringGrids[i] != database->currentGrid->neighbouringGrids[WrapNSEW(database->isFacing + facingOffset)]
        && !database->currentGrid->neighbouringGrids[i]->visited)
        {
            // push this node to the MM List
            for (int j = 0; j < MM_MAX; j++)
            {
                if (database->MMList[j] == NULL)
                {
                    database->MMList[j] = database->currentGrid->neighbouringGrids[i];
                    break;
                }
            }
        }
    }
}

// Need to remove MMListing after moving to there.
void RemoveFromMMList(Grid *MMgrid, Database *database)
{
    for (int i = 0; i < MM_MAX; ++i)
    {
        // After it has moved to the location of the MM List, remove it.
        if (MMgrid == database->MMList[i])
        {
            database->MMList[i] = NULL;
        }
    }
}