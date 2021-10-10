#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

void initWorldExporter(FILE *file);
void exportWorld(const int *world, int nRows, int nCols);

#endif
