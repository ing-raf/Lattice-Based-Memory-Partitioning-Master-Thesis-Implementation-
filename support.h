/*
 * Definitions of command - line and user interface and other miscellaneous 
 * helper functions. 
 */

#ifndef SUPPORT_H
#define SUPPORT_H

#include<stdio.h>

typedef struct {
	unsigned phase_num;
} phase;

phase * start(FILE *);
void new_phase(FILE *, phase *);
void complete_phase(FILE *, phase *);
void abort_phase(FILE *, phase *);
void error(FILE *, const char *);
void warning(FILE *, const char *);
void news(FILE *, const char *);
void info(FILE *, const char *, int);
void finish(FILE *, phase *);

#endif /* SUPPORT_H */
