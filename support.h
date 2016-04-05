/*
 * Definition of command - line and user interface and other miscellaneous 
 * helper functions. 
 */

#ifndef SUPPORT_H
#define SUPPORT_H

typedef struct {
	unsigned phase_num;
} phase;

phase * start();
void new_phase(phase *);
void complete_phase(phase *);
void abort_phase(phase *);
void error(const char *, phase *);
void finish(phase *);

#endif /* SUPPORT_H */
