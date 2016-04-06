/*
 * Implementation of command - line and user interface and other miscellaneous
 * helper functions
 */
#include<stdlib.h>
#include<stdio.h>

#include "support.h"
#include "colours.h"

const char * phaseNames[] = {
	"Reading input files",
	"Virtual address space allocation",
};

phase * start () {
	
	phase * newPhase = malloc(sizeof(phase));
	newPhase -> phase_num = 0;
	
	__SET_MAGENTA
	printf("Step %d) - %s", (newPhase -> phase_num) + 1, phaseNames[newPhase -> phase_num]);
	__SET_DEFAULT
	printf("\n");
	
	return newPhase;
}

void new_phase (phase * phasePtr) {
	
	phasePtr -> phase_num += 1;
	
	__SET_MAGENTA
	printf("Step %d) - %s", (phasePtr -> phase_num) + 1, phaseNames[phasePtr -> phase_num]);
	__SET_DEFAULT
	printf("\n");
}

void complete_phase (phase * phasePtr) {
	
	__SET_MAGENTA
	printf("Step %d) - %s - ", (phasePtr -> phase_num) + 1, phaseNames[phasePtr -> phase_num]);
	__SET_GREEN
	printf("Completed");
	__SET_DEFAULT
	printf("\n");
}


void abort_phase (phase * phasePtr) {
	
	__SET_MAGENTA
	printf("Step %d) - %s - ", (phasePtr -> phase_num) + 1, phaseNames[phasePtr -> phase_num]);
	__SET_RED
	printf("Failed");
	__SET_DEFAULT
	printf("\n");
	
	free(phasePtr);
	exit(-1);	
}

void error (const char * message, phase * phasePtr) {
	
	__SET_RED
	printf("%s", message);
	__SET_DEFAULT
	printf("\n");
	
	free(phasePtr);
	exit(-1);
} 

void finish (phase * phasePtr) {
	
	__SET_GREEN
	printf("Operation completed successfully");
	__SET_DEFAULT
	printf("\n");
	
	free(phasePtr);
	exit(0);
} 
