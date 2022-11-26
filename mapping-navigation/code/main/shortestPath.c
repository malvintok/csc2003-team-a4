#include <string.h>
#include "database.h"

/*Func prototypes*/
void InitAlgorithim(Database *d);
void recurToDest(Grid *start, Grid *end, Grid *come_from, int recursiveCall);
bool append2Array(Grid *array[], Grid *grid);
void popArray(Grid *array[]);
bool isLargerthan(Grid *array1[], Grid *array2[]);

/*Variables*/
// init an array of grid struct to all NULL
Grid *tempPath[GRID_MAX] = { NULL };
Database *database;

void InitAlgorithim(Database *d)
{
    database = d;
}
/**
 * @brief Function will takes in start grid and end grid: returns an array
 * of grid address that is the shortest path
 * @param start : Start grid's address
 * @param end : End grid's address
 * @param come_from: grid the prev recursive call was called from (can put as start grid address for first call)
 * @param recursiveCall 0. input it as 0, it is used to check the current recursion loop
 */
void recurToDest(Grid *start, Grid *end, Grid *come_from, int recursiveCall)
{
    // If current != (address of) end grid.
    if (start != end)
    {
        /*check which is the non-null neighbour grids, Max amount of linked grid is 4 (since NSEW)*/
        for (int i = 0; i < NSEW; i++)
        {
            /*
             *If either N,S,E or W is has a linked grid
             *AND
             *the linked grid is not a grid we just come from (this can happen because it is dependent on the order of NSEW)
             */
            if (start->neighbouringGrids[i] != NULL && start->neighbouringGrids[i] != come_from)
            {
                // if grid has been visited before or it is the destination
                if (start->neighbouringGrids[i]->visited == true || start->neighbouringGrids[i] == end)
                {
                    // append this grid to the tempPath array, if unable to append exit from this cycle
                    if(!append2Array(tempPath, start))
                        return;

                    // call recursive function with start as the new curr
                    recurToDest(start->neighbouringGrids[i], end, start, recursiveCall+1);
                    
                    popArray(tempPath);
                }
            }
        }
        
        // when recursiveCall is 0, it means all paths have been traversed
        if(recursiveCall == 0){
            // if path exist set pathfinding to true
            if(database->Path[0] != NULL){
                database->isPathfinding = true;
            }
            // reset tempPath path array
            memset(tempPath, 0, sizeof(tempPath));
        }
        return;
    }
    else  // if path found
    {
        // append the ending grid to the tempPath array
        append2Array(tempPath, end);

        // if path is empty or tempPath is shorter then path
        if(database->Path[0] == NULL || isLargerthan(database->Path, tempPath)){
            // replace path with the shorter one  
            memcpy(database->Path, tempPath, sizeof(tempPath));
        }
        popArray(tempPath);
        
        // return to previous reurToDest function call
        return;
    }
}

/**
 * @brief Function is used to append grid address to the an array of grid address
 *
 * @param array: Array to append to
 * @param grid: Item to append
 */
bool append2Array(Grid *array[], Grid *grid)
{
    int counter = 0;
    while (array[counter] != NULL)
    {
        if(array[counter] == grid)
            return false;
        counter++;
    }
    array[counter] = grid;
    return true;
}

/**
 * @brief Function is used to remove grid address from the top of array of grid address
 *
 * @param array: Array to pop from
 */
void popArray(Grid *array[])
{
    int counter = 0;
    do
    {
        counter++;
    }
    while (array[counter] != NULL);

    array[counter-1] = NULL;
    return;
}

/**
 * @brief compare two array length to see which has more elements
 * 
 * @param array1 LHS is larger than
 * @param array2 RHS
 */
bool isLargerthan(Grid *array1[], Grid *array2[])
{
    int length1 = 0;
    int length2 = 0;

    while(array1[length1] != NULL){
        length1++;
    }
    while(array2[length2] != NULL){
        length2++;
    }

    return length1 > length2;
}