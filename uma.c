#ifdef MOREVERBOSE
#define VERBOSE
#endif
/*
 * Implementation of the Lattice - Based Memory Mapping Banks Mapping techniquq
 * for the Uniform Memory Access time case
 */ 
#include<stdlib.h>

#ifdef VERBOSE
#include<stdio.h>
#endif

#include<pet.h>

#include "config.h"
#include "support.h"
#include "partitioning.h"

char ** validate_input(int, char**);

// Note that when an array lasts in Ptr, its elements are pointers
int main(int argc, char ** argv) {
	// Pointer to the current phase, used for graphical purposes
	phase * phasePtr = NULL;
	// Handle for the configuration of the isl and pet libraries
	isl_ctx * optionsHdl = NULL;
	// Array of task names
	char ** tasks = NULL;
	// Total numbers of tasks to work with
	unsigned numTasks = 0;
	// Array of the original polyhedral models of each task
	pet_scop ** polyhedralModelPtr = NULL;
	// Array containing the manipulated version of the polyhedral models of each task
	manipulated_polyhedral_model * modifiedPolyhedralModel = NULL;
	// Array containing the physical schedules
//	isl_union_map ** physicalSchedulePtr = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	
	// 1a) We check if the user passed some sources to work with 
	phasePtr = start();
	
	if (phasePtr == NULL) {
		error("Memory allocation problem :(");
		abort_phase(phasePtr);
	}
	
	if (argc <= 1) {
		error("Not enough input file(s)");
		abort_phase(phasePtr);
	}
	
	numTasks = argc - 1;
	
#ifdef VERBOSE
	printf("Overall number of tasks: %d\n", numTasks);
#endif
	
	tasks = validate_input(numTasks, argv);
	
	if (tasks == NULL) {
		printf("Too much tasks");
		abort_phase(phasePtr);
	}
	
#ifdef VERBOSE
	printf("Task names:\n");
	for (int i = 0; i < numTasks; i++)
		printf("%d)\t%s\n", i, tasks[i]);
#endif
	
	// 1b) Now we parse the input sources to get the whole polyhedral model
	optionsHdl = isl_ctx_alloc_with_pet_options();
	
	if (optionsHdl == NULL) {
		error("Sorry, there is something wrong with one of the libraries :(");
		abort_phase(phasePtr);
	}
	
	polyhedralModelPtr = malloc(numTasks * sizeof(pet_scop *));
	
	if (polyhedralModelPtr == NULL) {
		error("Memory allocation problem :(");
		return isl_stat_error;
	}
	
	outcome = parse_input(optionsHdl, tasks, polyhedralModelPtr, numTasks);
	
	if (outcome == isl_stat_error) {
		error("Error during parsing input files");
		abort_phase(phasePtr);
	}
	
	complete_phase(phasePtr);
	
	// 2) Virtual memory allocation 
	new_phase(phasePtr);
	
	modifiedPolyhedralModel = malloc(numTasks * sizeof(manipulated_polyhedral_model));
	
	if (modifiedPolyhedralModel == NULL) {
		error("Memory allocation problem :(");
		abort_phase(phasePtr);
	}
	
	outcome = virtual_allocation(optionsHdl, polyhedralModelPtr, modifiedPolyhedralModel, numTasks);
	
	if (outcome == isl_stat_error) {
		error("Error during virtual address space allocation");
		abort_phase(phasePtr);
	}
	
	complete_phase(phasePtr);
	
	// 3) Building the physical schedule
	new_phase(phasePtr);
	
//	physicalSchedulePtr = malloc(numTasks * sizeof(isl_union_map *));
	
//	if (physicalSchedulePtr == NULL) {
//		error("Memory allocation problem :(");
//		abort_phase(phasePtr);
//	}
	
	outcome = physical_schedule(optionsHdl, polyhedralModelPtr, modifiedPolyhedralModel, numTasks);
	
	if (outcome == isl_stat_error) {
		error("Error during physical schedule building");
		abort_phase(phasePtr);
	}
	
	complete_phase(phasePtr);
	
	// 4) Building the linearized schedule
	
	outcome = eliminate_parameters(modifiedPolyhedralModel, numTasks);
	
	if (outcome == isl_stat_error) {
		error("Error during parameter projection out");
		abort_phase(phasePtr);
	}
	
//	linearize_dates(physicalSchedulePtr, numTasks);
	
	// Be clean
//	free(physicalSchedulePtr);
	free(modifiedPolyhedralModel);
	free(tasks);
	finish(phasePtr);
}

char ** validate_input(int n, char ** argv) {
	
	if (n > MAXTASKS) 
		return NULL;
	
	char ** names = malloc(n * sizeof(char *));
	
	for (int i = 0; i < n; i++)
		names[i] = argv[i+1];
	
	return names;
}