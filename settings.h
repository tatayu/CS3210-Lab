#ifndef SETTINGS_H
#define SETTINGS_H

/**
 * If set to 0, disables compilation of the code that exports each generation to the
 * file specified by the optional fourth command-line argument.
 * 
 * The exported file can be passed as-is to the GOI visualizer.
 * 
 * If set to a non-zero value, enables compilation of aforementioned code.
 * 
 * Note that if no fourth command-line argument is specified, export will not occur whether
 * or not this is enabled. But if you are extremely interested in performance, then you
 * might care to not even have this compiled in for your final build.
 * 
 * This may be helpful for your debugging efforts (or simply fun to watch in the visualizer) but you need not use it.
 * 
 * Instructions and executables for the GOI visualizer are here:
 * https://drive.google.com/drive/folders/1bzIiDBobBtCSQtgwySVJINGwbJ6_c1XJ?usp=sharing
 */
#define EXPORT_GENERATIONS 0

/**
 * If set to 0, does nothing.
 * 
 * If set to a non-zero value, prints every generation (including starting generation) to standard output.
 * 
 * This may be helpful for your debugging efforts but you need not use it.
 */
#define PRINT_GENERATIONS 0

#endif
