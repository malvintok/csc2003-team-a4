#include <string.h>
#include "database.h"

/*Func prototypes*/
void InitAlgorithim(Database *d);
void recurToDest(Grid *start, Grid *end, Grid *come_from);
bool append2Array(Grid *array[], Grid *grid);
void popArray(Grid *array[]);

/*Variables*/
// init an array of grid struct to all NULL
Database *database;
void InitAlgorithim(Database *d)
{
    database = d;
}
/**
 * @brief Function will takes in start grid and end grid: returns an array
 * of grid address that is the first path found
 * @param start : Start grid's address
 * @param end : End grid's address
 * @param come_from: grid the prev recursive call was called from (can put as start grid address for first call)
 * @return void:
 */
void recurToDest(Grid *start, Grid *end, Grid *come_from)
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
                    // append this grid to the path array, if unable to append exit from this cycle
                    if(!append2Array(database->Path, start))
                        return;

                    // call recursive function with start as the new curr
                    recurToDest(start->neighbouringGrids[i], end, start);

                    // if path found exit recursion
                    if(database->isPathfinding)
                        return;
                    else
                        popArray(database->Path);
                }
            }
        }
        return;
    }
    else // if path found
    {
        // append the ending grid to the path array
        append2Array(database->Path, end);
        database->isPathfinding = true;
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
        // if grid is already in array, return false
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