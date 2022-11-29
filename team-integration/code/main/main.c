// Team Mapping and Navigation

// switch pathfining algo with the folowing define
// define "SHORTEST_PATH" to run the shortest path algo
// remove the define to run the first path found algo
#define SHORTEST_PATH

#ifdef SHORTEST_PATH
#include "pathfinding\shortestPath.c"
#else
#include "pathfinding\firstPathFound.c"
#endif

#include "database.h"
#include "motor.c"
#include "ultrasonic.c"
#include "barcodencoder.c"
#include "comms.c"

// Finite state machine states for maping
typedef enum FINITE_STATE
{
    FS_CHECKING_GRID = 0, // performs checks on current grid and decides next course of action
    FS_GOING_STRAIGHT, // when the car is moving straight
    FS_TURN_RIGHT, // when the car is turing right
    FS_TURN_LEFT, // when the car is turing left
    FS_TURN_AROUND, // when the car is doing a 180 degree turn
    FS_PATHFINDING, // state where the car navigates to a selected grid
    FS_FINISH_MAPPING // ending state when the entire map is maped out

} FINITE_STATE;

// corosponding string name for the DIRECTION enum found in database
const char *direction_str[NSEW] = {"North", "East", "South", "West"};

struct repeating_timer updateCommsTimer;

// Function Prototypes

void PicoStartUpTest();
void CheckPosibleMoves(int L, int R, int F);
void LinkCurrentGrid(int facingOffset);
Grid *GetGrid(int x, int y);
Grid *GetNextAvailableGrid(int nextX, int nextY);
void SetCoordinate(int *x, int *y, DIRECTION facing);
int FindTurningIndex(Grid *grid);
void UpdateMMList(int facingOffset);
void RemoveFromMMList(Grid *MMgrid);
bool UpdateComms(struct repeating_timer *t);

unsigned char mygetchar()
{
    int c;
    while ((c = getchar_timeout_us(0)) < 0);
    return (unsigned char)c;
};

int main()
{
    // Init all standard I/O
    stdio_init_all();

    // FSM current state
    FINITE_STATE curState = FS_CHECKING_GRID;
    // flag to toggle when finish mapping
    bool finishMapping = false;

    // shared database for all modules to use
    Database db;
    database = &db;
    memset(database, 0, sizeof(Database));

    // set the current grid to slot 0 of the grid list
    database->currentGrid = &(database->gridList[0]);
    database->currentGrid->visited = true;

    // initialize other modules 
    init_Motor();

    init_Barcodencoder();

    init_Comms();

    init_Ultrasonic();

    // timer to update comms
    add_repeating_timer_ms(UPDATE_COMMS_INTERVAL, UpdateComms, NULL, &updateCommsTimer);

    // bink led lights to signify start of excecution
    PicoStartUpTest();
    // mygetchar();

    // Main loop, will keep running
    while(1)
    {
        // Final State Machine (FSM)
        switch (curState)
        {

        // Moving forward or just moving around?
        case FS_GOING_STRAIGHT:
        {
            printf("\n<GOING_STRAIGHT State>\n");

            while (motor_actionCode){}
            sleep_ms(3000);

            // update current grid to next grid based on the direction of car
            database->currentGrid = database->currentGrid->neighbouringGrids[database->isFacing];
            database->currentGrid->visited = true;

            curState = FS_CHECKING_GRID;
        }
        break;

        case FS_TURN_LEFT:
        {
            printf("\n<TURN_LEFT State>\n");
            database->isFacing = WrapNSEW(database->isFacing + LEFT);

            while (motor_actionCode){}
            sleep_ms(3000);

            motors_moveByNotch(GRID_NOTCH);
            curState = FS_GOING_STRAIGHT;
        }
        break;

        case FS_TURN_RIGHT:
        {
            printf("\n<TURN_RIGHT State>\n");
            database->isFacing = WrapNSEW(database->isFacing + RIGHT);

            while (motor_actionCode){}
            sleep_ms(3000);

            motors_moveByNotch(GRID_NOTCH);
            curState = FS_GOING_STRAIGHT;
        }
        break;

        case FS_TURN_AROUND:
        {
            printf("\n<TURN_AROUND State>\n");
            database->isFacing = WrapNSEW(database->isFacing + BEHIND);
            
            while (motor_actionCode){}
            sleep_ms(3000);

            if (database->isPathfinding == true)
            {
                motors_moveByNotch(GRID_NOTCH);
                curState = FS_GOING_STRAIGHT;
            }
            else
                curState = FS_CHECKING_GRID;
        }
        break;

        case FS_CHECKING_GRID:
        {
            printf("\n<CHECKING_GRID State>\n");

            printf("\nCurrent gridList array slot: %d\n", database->gridCounter);
            printf("Current position x: %d, y: %d\n", database->currentGrid->x, database->currentGrid->y);
            printf("Currently facing: %s\n", direction_str[database->isFacing]);

            if (database->isPathfinding)
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
            float uInputF = .0, uInputL = .0, uInputR = .0;
            
            database->ultraDist = uInputF = getDist(CENTER_TRIG, CENTER_ECHO);
            uInputL = getDist(LEFT_TRIG, LEFT_ECHO);
            uInputR = getDist(RIGHT_TRIG, RIGHT_ECHO);

            printf("Front Sonic: %.3f\n", uInputF);
            printf("Left Sonic: %.3f\n", uInputL);
            printf("Right Sonic: %.3f\n", uInputR);

            //send ultrasonic distance to comms
            // sendDist(uInputF);

            // based on ultrasonic input, update the current grid's details and other maping infomation
            CheckPosibleMoves(uInputF, uInputL, uInputR);
            

            // after updating data above
            // check which direction is possible. Check Right, Left then Front.
            // check current grid's right side neighbour (isfacing+1) is not NULL (has a grid address) and that it is not yet visited
            if (database->currentGrid->neighbouringGrids[WrapNSEW(database->isFacing + RIGHT)] != NULL && !database->currentGrid->neighbouringGrids[WrapNSEW(database->isFacing + RIGHT)]->visited)
            {
                UpdateMMList(RIGHT);
		        motors_turnMotorsNew(0,90);
                curState = FS_TURN_RIGHT;
            }
            // check current grid's left side neighbour (isfacing-1) is not NULL (has a grid address) and that it is not yet visited
            else if (database->currentGrid->neighbouringGrids[WrapNSEW(database->isFacing + LEFT)] != NULL && !database->currentGrid->neighbouringGrids[WrapNSEW(database->isFacing + LEFT)]->visited)
            {
                UpdateMMList(LEFT);
		        motors_turnMotorsNew(1,90);
                curState = FS_TURN_LEFT;
            }
            // check current grid's front neighbour is not NULL (has a grid address) and that it is not yet visited
            else if (database->currentGrid->neighbouringGrids[(database->isFacing)] != NULL && !database->currentGrid->neighbouringGrids[(database->isFacing)]->visited)
            {
                motors_moveByNotch(GRID_NOTCH);
                barcodeReset();
                curState = FS_GOING_STRAIGHT;
            }
            else // Cannot move forward. Left, Right and Front no possible move.
            {
                // if able back is of current grid is null, turn 180 on the spot to perform ultrasonic check the time it enter this state
                if (database->currentGrid->neighbouringGrids[WrapNSEW(database->isFacing + BEHIND)] == NULL)
                {
		            motors_turnMotorsNew(0,180);
                    curState = FS_TURN_AROUND;
                    continue;
                }

                // When neighbors are already visited, start Dijkstra to navigate Multi-Move
                // Currently is only recursive path finding.

                // look for a multi move grid to go to
                Grid *toMoveto = NULL;
                for (int i = 0; i < MM_MAX; i++)
                {
                    if (database->MMList[i] != NULL && database->MMList[i] != database->currentGrid)
                    {
                        toMoveto = database->MMList[i];
                        break;
                    }
                }

                // if there is no multi move grid left, maping is complete
                if (toMoveto == NULL)
                {
                    printf("\nMM List is empty. Completed Mapping.\n");
                    finishMapping = true;
                    continue;
                }

                // perform path finding search to the multi move grid
                #ifdef SHORTEST_PATH
                recurToDest(database->currentGrid, toMoveto, database->currentGrid, 0);
                #else
                recurToDest(database->currentGrid, toMoveto, database->currentGrid);
                #endif
            }
        }
        break;

        case FS_PATHFINDING:
        {
            printf("\n<PATHFINDING State>\n");

            // counter to track the next grid to move to in path array
            // starts from 1 because index 0 is the current grid
            static int i = 1;
            
            if (database->Path[i] == NULL)
            {
                // We have travelled at the end of the path.
                
                // clean up / reset data and go back to check grid state
                RemoveFromMMList(database->Path[i - 1]);
                database->isPathfinding = false;
                memset(&database->Path, 0, sizeof(database->Path));
                i = 1;

                curState = FS_CHECKING_GRID;
            }
            else
            {
                // print path
                printf("\nRemainding path to take: ");
                for (int j = i; j < GRID_MAX; ++j)
                {
                    if (database->Path[j] == NULL)
                        break;
                    printf("[x:%d, y:%d]->", database->Path[j]->x, database->Path[j]->y);
                }
                printf("DONE\n");

                // Based on the car's current direction, find out the direction to turn to move to the next grid
                switch (FindTurningIndex(database->Path[i++]))
                {
                case FRONT:
                    motors_moveByNotch(GRID_NOTCH);
                    curState = FS_GOING_STRAIGHT;
                    break;
                case RIGHT:
		            motors_turnMotorsNew(0,90);
                    curState = FS_TURN_RIGHT;
                    break;
                case LEFT:
		            motors_turnMotorsNew(1,90);
                    curState = FS_TURN_LEFT;
                    break;
                case BEHIND:
                    motors_turnMotorsNew(0, 180);
                    curState = FS_TURN_AROUND;
                    break;
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

            // finish maping, get x, y input for going to another grid
            printf("Enter coordinates for X: ");
            userInput = mygetchar();
            if (userInput == '-')
            {
                printf("%c", userInput);
                pathFindingX = -1;
                userInput = mygetchar();
            }
            printf("%c", userInput);
            pathFindingX *= (userInput - '0');

            printf("\nEnter coordinates for Y: ");
            userInput = mygetchar();
            if (userInput == '-')
            {
                printf("%c", userInput);
                userInput = mygetchar();
                pathFindingY = -1;
            }
            printf("%c", userInput);
            pathFindingY *= (userInput - '0');
            
            
            // Start Path Finding
            // get grid based on input 
            // if grid exist and is not current grid, start path fining
            Grid *pFinder = GetGrid(pathFindingX, pathFindingY);
            if (pFinder != NULL && database->currentGrid != pFinder)
            {
                // perform path finding search selected grid
                #ifdef SHORTEST_PATH
                recurToDest(database->currentGrid, pFinder, database->currentGrid, 0);
                #else
                recurToDest(database->currentGrid, pFinder, database->currentGrid);
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

/**
 * @brief Based on ultra sensors inputs, update the current grid and map acordingly.
 * F, L, R are the front left and right sensor values
 */
void CheckPosibleMoves(int F, int L, int R)
{
    // for each of the sensors value's are above a a trashold,
    // it means that there is a posible path in that direction.
    // link the current grid with it
    if (F > DIST_FROM_WALL)
    {
        LinkCurrentGrid(FRONT);
    }
    if (L > DIST_FROM_WALL)
    {
        LinkCurrentGrid(LEFT);
    }
    if (R > DIST_FROM_WALL)
    {
        LinkCurrentGrid(RIGHT);
    }
}

/**
 * @brief Link current grid and its neighbour together, also set other maping info such as adding to multi move array
 * 
 * @param facingOffset offset of the neighbouring grid based on the current facing direction.
 * -1 for left, 0 for front, 1 for right
 */
void LinkCurrentGrid(int facingOffset)
{
    // Get current car facing direction and shift it to direction of "empty" grid
    // Modulus 4(NSEW) is to determine new direction (enum roll over).
    DIRECTION tempFacing = WrapNSEW(database->isFacing + facingOffset);

    // if grid is already linked, return
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
    Grid *nextGrid = GetNextAvailableGrid(tempX, tempY);
    // Put neighbour grid's address to current grid
    database->currentGrid->neighbouringGrids[tempFacing] = nextGrid;
    // Put the neighbouring grid to the current grid
    nextGrid->neighbouringGrids[WrapNSEW(tempFacing + BEHIND)] = database->currentGrid;

    // Set the x and y of the neighbouring grid
    nextGrid->x = tempX;
    nextGrid->y = tempY;
}

/**
 * @brief offset x, y position based on given direction.
 * north/south will ofset y axis
 * east west will offset x axis
 */
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

/**
 * @brief get address of an existing grid by x and y coordinates.
 */
Grid *GetGrid(int x, int y)
{
    for (int i = 0; i <= database->gridCounter; ++i)
    {
        if (x == database->gridList[i].x && y == database->gridList[i].y)
            return &database->gridList[i];
    }
    return NULL;
}

/**
 * @brief get address of an existing grid by x and y coordinates.
 * If not found, return a new grid
 */
Grid *GetNextAvailableGrid(int nextX, int nextY)
{
    for (int i = 0; i <= database->gridCounter; ++i)
    {
        if (nextX == database->gridList[i].x && nextY == database->gridList[i].y)
            return &database->gridList[i];
    }

    database->gridCounter += 1;

    if (database->gridCounter >= GRID_MAX)
    {
        printf("\n< ERROR! RAN OUT OF GRIDS >\n");
        exit(404);
    }

    return &database->gridList[database->gridCounter];
}

/**
 * @brief based on the car's current grid and direction, find out the offset direction towards the selected grid
 * @param grid the grid to compare with
 */
int FindTurningIndex(Grid *grid)
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
        turningIndex = LEFT;
    else if (turningIndex == -2)
        turningIndex = BEHIND;
    else if (turningIndex == -3)
        turningIndex = RIGHT;

    return turningIndex;
}

/**
 * @brief Called before car is about to move to next grid.
 * adds any other posible neighbour grids that has not been visited to multi move list.
 * 
 * @param facingOffset offset of the grid the car is turning towards based on the current facing direction.
 * -1 for left, 0 for front, 1 for right
 * 
 */
void UpdateMMList(int facingOffset)
{
    for (int i = 0; i < NSEW; i++)
    {
        // if there is a grid other then right side of current
        if (database->currentGrid->neighbouringGrids[i] != NULL && database->currentGrid->neighbouringGrids[i] != database->currentGrid->neighbouringGrids[WrapNSEW(database->isFacing + facingOffset)] && !database->currentGrid->neighbouringGrids[i]->visited)
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

/**
 * 
 * @brief Removes a grid from the multi move list
 * 
 */
void RemoveFromMMList(Grid *MMgrid)
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

/**
 * 
 * @brief Send data to comms
 * 
 */
bool UpdateComms(struct repeating_timer *t)
{
    
    sendDist(database->ultraDist);

    sendWheelSpeedL(database->wheelSpeedL);

    sendWheelSpeedR(database->wheelSpeedR);

    if (database->barcodeResult != NULL)
    {
        sendBarcode(database->barcodeResult);
        sendBarcodeCoord(database->currentGrid->x, database->currentGrid->y);
        database->barcodeResult = NULL;
    }

    // sendDist(distancefromsensor2);

    // sendHeight(0);

    // sendHumpCoord(0, 0);

    sendCoord(database->currentGrid->x, database->currentGrid->y,
              database->currentGrid->neighbouringGrids[0] == NULL ? 0 : 1,
              database->currentGrid->neighbouringGrids[1] == NULL ? 0 : 1,
              database->currentGrid->neighbouringGrids[2] == NULL ? 0 : 1,
              database->currentGrid->neighbouringGrids[3] == NULL ? 0 : 1);
            
}