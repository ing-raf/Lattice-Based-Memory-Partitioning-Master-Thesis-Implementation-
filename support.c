/*
 * Implementation of command - line and user interface and other miscellaneous
 * helper functions
 */
#include<stdlib.h>

#include "support.h"
#include "colours.h"

const char * phaseNames[] = {
	"Reading input files",
	"Virtual address space allocation",
	"Reading lattices",
	"Physical schedule building",
	"Allocation building",
	"Concurrent dates computation",
	"Instant local slice building",
	"Mapping parameters computation",
	"Solution space evaluation"
};

phase * start (FILE * stream) {
	
	phase * newPhase = malloc(sizeof(phase));
	newPhase -> phase_num = 0;
	
	__SET_MAGENTA
	fprintf(stream, "Step %d) - %s", (newPhase -> phase_num) + 1, phaseNames[newPhase -> phase_num]);
	__SET_DEFAULT
	fprintf(stream, "\n");
	
	return newPhase;
}

void new_phase (FILE * stream, phase * phasePtr) {
	
	phasePtr -> phase_num += 1;
	
	__SET_MAGENTA
	fprintf(stream, "Step %d) - %s", (phasePtr -> phase_num) + 1, phaseNames[phasePtr -> phase_num]);
	__SET_DEFAULT
	fprintf(stream, "\n");
}

void complete_phase (FILE * stream, phase * phasePtr) {
	
	__SET_MAGENTA
	fprintf(stream, "Step %d) - %s - ", (phasePtr -> phase_num) + 1, phaseNames[phasePtr -> phase_num]);
	__SET_GREEN
	fprintf(stream, "Completed");
	__SET_DEFAULT
	fprintf(stream, "\n");
}


void abort_phase (FILE * stream, phase * phasePtr) {
	
	__SET_MAGENTA
	fprintf(stream, "Step %d) - %s - ", (phasePtr -> phase_num) + 1, phaseNames[phasePtr -> phase_num]);
	__SET_RED
	fprintf(stream, "Failed");
	__SET_DEFAULT
	fprintf(stream, "\n");
	
	free(phasePtr);
	exit(-1);	
}

void error (FILE * stream, const char * message) {
	
	__SET_RED
	fprintf(stream, "%s", message);
	__SET_DEFAULT
	fprintf(stream, "\n");
	fflush(stream);
	
} 

void warning (FILE * stream, const char * message) {
	
	__SET_YELLOW
	fprintf(stream, "%s", message);
	__SET_DEFAULT
	fprintf(stream, "\n");
	fflush(stream);
	
} 

void info (FILE * stream, const char * message, int num) {
	
	__SET_BLUE
	fprintf(stream, message, num);
	__SET_DEFAULT
	fprintf(stream, "\n");
	
} 

void finish (FILE * stream, phase * phasePtr) {
	
	__SET_GREEN
	fprintf(stream, "Operation completed successfully");
	__SET_DEFAULT
	fprintf(stream, "\n");
	
	free(phasePtr);
	
	if(stream != stdout)
		fclose(stream);
	exit(0);
} 
