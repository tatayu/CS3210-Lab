/**
 * NOTE TO STUDENTS: you should not need to modify this file.
 * You can if you wish to, but incorrectly modifying it may lead to an incorrect program which you will be penalized for.
 * 
 * Also, if you modify this file (e.g. for some optimization) and wish it to be taken into consideration for grading,
 * please make a note of it in your report. Otherwise, we will NOT consider it when grading.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include "util.h"
//#include "exporter.h"
#include "settings.h"
#include "goi_cuda.h"
    

int readParam(FILE *fp, char **line, size_t *len, int *param);
int readWorldLayout(FILE *fp, char **line, size_t *len, int *world, int nRows, int nCols);

/**
 * Handles input, output and file open/close operations. Delegates simulation to goi.
 */
int main(int argc, char *argv[])
{
    int nGenerations;
    int nRows;
    int nCols;
    int *startWorld;
    int nInvasions;
    int *invasionTimes;
    int **invasionPlans;
    //int nThreads;
    int gridX;
    int gridY;
    int gridZ;
    int blockX;
    int blockY;
    int blockZ;

    FILE *outputFile;
    FILE *inputFile;
    char *line = NULL;
    size_t len = 0;

    //GRID_X, GRID_Y, GRID_Z, BLOCK_X, BLOCK_Y, BLOCK_Z

    if (argc < 8)
    {
#if EXPORT_GENERATIONS
        fprintf(stderr, "Usage: %s <INPUT_PATH> <OUTPUT_PATH> <NUM_THREADS> [<OPT_EXPORT_PATH>]\n", argv[0]);
#else
        fprintf(stderr, "Usage: %s <INPUT_PATH> <OUTPUT_PATH> <GRID_X> <GRID_Y> <GRID_Z> <BLOCK_X> <BLOCK_Y> <BLOCK_Z> \n", argv[0]);
#endif
        exit(EXIT_FAILURE);
    }

    printf("<INPUT_PATH>: %s\n", argv[1]);
    printf("<OUTPUT_PATH>: %s\n", argv[2]);
    printf("<GRID_X>: %s\n", argv[3]);
    printf("<GRID_Y>: %s\n", argv[4]);
    printf("<GRID_Z>: %s\n", argv[5]);
    printf("<BLOCK_X>: %s\n", argv[6]);
    printf("<BLOCK_Y>: %s\n", argv[7]);
    printf("<BLOCK_Z>: %s\n", argv[8]);

#if EXPORT_GENERATIONS
    FILE *exportFile = NULL;
    if (argc >= 5)
    {
        printf("<OPT_EXPORT_PATH>: %s\n", argv[4]);
        exportFile = fopen(argv[4], "w");
        initWorldExporter(exportFile);
    }
#endif

    inputFile = fopen(argv[1], "r");
    if (inputFile == NULL)
    {
        fprintf(stderr, "Failed to open %s for reading. Aborting...\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    outputFile = fopen(argv[2], "w");
    if (outputFile == NULL)
    {
        fprintf(stderr, "Failed to open %s for writing. Aborting...\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    // Parse GRID and BLOCK
    if (sscanf(argv[3], "%d", &gridX) != 1 )
    {
        fprintf(stderr, "Failed to parse <GRID_X> as positive integer. Got '%s'. Aborting...\n", argv[3]);
        exit(EXIT_FAILURE);
    }

    if (sscanf(argv[4], "%d", &gridY) != 1 )
    {
        fprintf(stderr, "Failed to parse <GRID_Y> as positive integer. Got '%s'. Aborting...\n", argv[3]);
        exit(EXIT_FAILURE);
    }
    
    if (sscanf(argv[5], "%d", &gridZ) != 1 )
    {
        fprintf(stderr, "Failed to parse <GRID_Z> as positive integer. Got '%s'. Aborting...\n", argv[3]);
        exit(EXIT_FAILURE);
    }

    if (sscanf(argv[6], "%d", &blockX) != 1 )
    {
        fprintf(stderr, "Failed to parse <BLOCK_X> as positive integer. Got '%s'. Aborting...\n", argv[3]);
        exit(EXIT_FAILURE);
    }

    if (sscanf(argv[7], "%d", &blockY) != 1 )
    {
        fprintf(stderr, "Failed to parse <BLOCK_Y> as positive integer. Got '%s'. Aborting...\n", argv[3]);
        exit(EXIT_FAILURE);
    }

    if (sscanf(argv[8], "%d", &blockZ) != 1 )
    {
        fprintf(stderr, "Failed to parse <BLOCK_Z> as positive integer. Got '%s'. Aborting...\n", argv[3]);
        exit(EXIT_FAILURE);
    }
    
    if (gridX < 1 || gridY < 1 || gridZ < 1 || blockX < 1 || blockY < 1 || blockZ < 1)
    {
        fprintf(stderr, "GRID/BLOCK dimension has invalid value: %d %d %d %d %d %d. Aborting...\n", gridX, gridY, gridZ, blockX, blockY, blockZ);
        exit(EXIT_FAILURE);
    }

    // Read nGenerations
    if (readParam(inputFile, &line, &len, &nGenerations) == -1)
    {
        fprintf(stderr, "Failed to read N_GENERATIONS. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    // Read nRows
    if (readParam(inputFile, &line, &len, &nRows) == -1)
    {
        fprintf(stderr, "Failed to read N_ROWS. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    // Read nCols
    if (readParam(inputFile, &line, &len, &nCols) == -1)
    {
        fprintf(stderr, "Failed to read N_COLS. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    if (nRows == 0 || nCols == 0)
    {
        fprintf(stderr, "N_ROWS or N_COLS is 0. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    // Read start world
    startWorld = malloc(sizeof(int) * nRows * nCols);
    if (startWorld == NULL || readWorldLayout(inputFile, &line, &len, startWorld, nRows, nCols) == -1)
    {
        fprintf(stderr, "Failed to read STARTING_WORLD. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    // Read nInvasions
    if (readParam(inputFile, &line, &len, &nInvasions) == -1)
    {
        fprintf(stderr, "Failed to read N_INVASIONS. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    // Read invasions
    invasionTimes = malloc(sizeof(int) * nInvasions);
    invasionPlans = malloc(sizeof(int *) * nInvasions);
    if (invasionTimes == NULL || invasionPlans == NULL)
    {
        fprintf(stderr, "No memory for invasions. Aborting...\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < nInvasions; i++)
    {
        if (invasionTimes == NULL || readParam(inputFile, &line, &len, invasionTimes + i))
        {
            fprintf(stderr, "Failed to read INVASION_TIME. Aborting...\n");
            exit(EXIT_FAILURE);
        }

        invasionPlans[i] = malloc(sizeof(int) * nRows * nCols);
        if (invasionPlans[i] == NULL || readWorldLayout(inputFile, &line, &len, invasionPlans[i], nRows, nCols))
        {
            fprintf(stderr, "Failed to read INVASION_PLAN. Aborting...\n");
            exit(EXIT_FAILURE);
        }
    }

#if PRINT_GENERATIONS
    printf("N_GENERATIONS: %d, N_ROWS: %d, N_COLS: %d, N_INVASIONS: %d\n", nGenerations, nRows, nCols, nInvasions);
    printf("\n== STARTING_WORLD ==\n");
    printWorld(startWorld, nRows, nCols);
    for (int i = 0; i < nInvasions; i++)
    {
        printf("\n== invasion %d at time: %d ==\n", i, invasionTimes[i]);
        printWorld(invasionPlans[i], nRows, nCols);
    }
#endif

    // we're done with the file
    fclose(inputFile);
    if (line)
    {
        free(line);
    }

    // run the simulation
    int warDeathToll = goi(gridX, gridY, gridZ, blockX, blockY, blockZ, nGenerations, startWorld, nRows, nCols, nInvasions, invasionTimes, invasionPlans);

    // output the result
    fprintf(outputFile, "%d", warDeathToll);
    fclose(outputFile);

#if EXPORT_GENERATIONS
    if (exportFile != NULL)
    {
        fclose(exportFile);
    }
#endif

    // free everything!
    for (int i = 0; i < nInvasions; i++)
    {
        free(invasionPlans[i]);
    }
    free(invasionTimes);
    free(invasionPlans);
    free(startWorld);
}

// readParam reads one integer from a line into param, advancing the read head to the next line.
// -1 is returned on error.
int readParam(FILE *fp, char **line, size_t *len, int *param)
{
    if (getline(line, len, fp) == -1 ||
        sscanf(*line, "%d", param) != 1)
    {
        return -1;
    }
    return 0;
}

// readWorldLayout reads a world layout specified by nRows and nCols, advancing the read head by
// nRows number of lines. -1 is returned on error.
int readWorldLayout(FILE *fp, char **line, size_t *len, int *world, int nRows, int nCols)
{
    for (int row = 0; row < nRows; row++)
    {
        if (getline(line, len, fp) == -1)
        {
            return -1;
        }

        char *p = *line;
        for (int col = 0; col < nCols; col++)
        {
            char *end;
            int cell = strtol(p, &end, 10);

            // unexpected end
            if (cell == 0 && end == p)
            {
                return -1;
            }

            // other errors
            if (errno == EINVAL || errno == ERANGE)
            {
                return -1;
            }

            setValueAt(world, nRows, nCols, row, col, cell);
            p = end;
        }
    }

    return 0;
}
