#ifndef GOI_CUDA_H
#define GOI_CUDA_H


int goi(int gridX, int gridY, int gridZ, int blockX, int blockY, int blockZ, int nGenerations, const int *startWorld, int nRows, int nCols, int nInvasions, const int *invasionTimes, int **invasionPlans);

#endif
