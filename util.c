/**
 * NOTE TO STUDENTS: you should not need to modify this file.
 * You can if you wish to, but incorrectly modifying it may lead to an incorrect program which you will be penalized for.
 * 
 * Also, if you modify this file (e.g. for some optimization) and wish it to be taken into consideration for grading,
 * please make a note of it in your report. Otherwise, we will NOT consider it when grading.
 */

#include "util.h"
#include <stdio.h>

/**
 * returns the value at the input row and col of the input grid, if valid.
 * 
 * -1 is returned if row or col is out of bounds (as specified by nRows and nCols).
 */
int getValueAt(const int *grid, int nRows, int nCols, int row, int col)
{
    if (row < 0 || row >= nRows || col < 0 || col >= nCols)
    {
        return -1;
    }

    return *(grid + (row * nCols) + col);
}

/**
 * sets the value at the input row and col of the input grid to val.
 * 
 * Does nothing if row or col is out of bounds (as specified by nRows and nCols).
 */
void setValueAt(int *grid, int nRows, int nCols, int row, int col, int val)
{
    if (row < 0 || row >= nRows || col < 0 || col >= nCols)
    {
        return;
    }

    *(grid + (row * nCols) + col) = val;
}

/**
 * Writes the input world to stdout.
 */
void printWorld(const int *world, int nRows, int nCols)
{
    for (int row = 0; row < nRows; row++)
    {
        for (int col = 0; col < nCols; col++)
        {
            printf("%d ", *(world + (row * nCols) + col));
        }
        printf("\n");
    }
}
