/*
 * Declaration of architecture - specific parameters
 */

#ifndef CONFIG_H
#define CONFIG_H

extern const int moreIndent;
extern const int lessIndent;

extern const unsigned MAXTASKS; // to check if there are enough configurations for the provided input
extern const unsigned N[];
extern const unsigned NUMPARAMS[]; // to support for tasks with different parameters
extern const unsigned PARAMS[2][2];
extern const unsigned NUMBANKS;

#endif /* CONFIG_H */
