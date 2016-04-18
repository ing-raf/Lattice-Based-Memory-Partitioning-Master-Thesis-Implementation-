#ifdef MOREVERBOSE
#define VERBOSE
#endif
/*
 * Implementation of the Lattice - Based Memory Mapping Banks Mapping techniquq
 * for the Uniform Memory Access time case
 */ 
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#include<pet.h>

#include "config.h"
#include "support.h"
#include "model.h"
#include "partitioning.h"

const unsigned OPTIONS = 1;

char ** validate_input(int, char**);

// Note that when an array lasts in Ptr, its elements are pointers
int main(int argc, char ** argv) {
	// Pointer to the current phase, used for graphical purposes
	phase * phasePtr = NULL;
	// Handle to the output stream
	FILE * outputStreamHdl = NULL;
	// Handle for the configuration of the isl and pet libraries
	isl_ctx * optionsHdl = NULL;
	// Array of task names
	char ** tasks = NULL;
	// Total numbers of tasks to work with
	unsigned numTasks = 0;
	// Array of the original polyhedral models of each task
	pet_scop ** polyhedralModelPtr = NULL;
	// Array containing the manipulated version of the polyhedral models of each task
	manipulated_polyhedral_model ** modifiedPolyhedralModel = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	
	// 1a) We check if the user passed some sources to work with 
	if (argc <= OPTIONS + 1) {
		perror("Not enough input file(s)");
		exit(1);
	}
	
	
	if (strcmp(argv[1], "stdout") == 0)
		outputStreamHdl = stdout;
	else {
		outputStreamHdl = fopen(argv[1],"w");
		
		if (outputStreamHdl == NULL) {
			perror("Cannot create the output file");
			exit(1);
		}
	}
	
	phasePtr = start(outputStreamHdl);
	
	if (phasePtr == NULL) {
		error(outputStreamHdl, "Memory allocation problem :(");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	numTasks = argc - OPTIONS - 1;
	
	tasks = validate_input(numTasks, argv);
	
	if (tasks == NULL) {
		fprintf(outputStreamHdl, "Memory allocation problem :(");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	
#ifdef VERBOSE
	fprintf(outputStreamHdl, "Overall number of tasks: %d\n", numTasks);
	fprintf(outputStreamHdl, "Task names:\n");
	for (int i = 0; i < numTasks; i++)
		fprintf(outputStreamHdl, "%d)\t%s\n", i, tasks[i]);
#endif
	
	// 1b) Now we parse the input sources to get the whole polyhedral model
	optionsHdl = isl_ctx_alloc_with_pet_options();
	
	if (optionsHdl == NULL) {
		error(outputStreamHdl, "Sorry, there is something wrong with one of the libraries :(");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	polyhedralModelPtr = malloc(numTasks * sizeof(pet_scop *));
	
	if (polyhedralModelPtr == NULL) {
		error(outputStreamHdl, "Memory allocation problem :(");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	outcome = parse_input(outputStreamHdl, optionsHdl, tasks, polyhedralModelPtr, numTasks);
	
	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during parsing input files");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	complete_phase(outputStreamHdl, phasePtr);
	
	// 2) Virtual memory allocation 
	new_phase(outputStreamHdl, phasePtr);
	
	modifiedPolyhedralModel = manipulated_polyhedral_model_array_alloc(numTasks);
	
	if (modifiedPolyhedralModel == NULL) {
		error(outputStreamHdl, "Memory allocation problem :(");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	outcome = virtual_allocation(outputStreamHdl, optionsHdl, polyhedralModelPtr, modifiedPolyhedralModel, numTasks);
	
	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during virtual address space allocation");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	complete_phase(outputStreamHdl, phasePtr);
	
	// 3) Building the physical schedule
	new_phase(outputStreamHdl, phasePtr);
	
	outcome = physical_schedule(outputStreamHdl, optionsHdl, polyhedralModelPtr, modifiedPolyhedralModel, numTasks);
	
	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during physical schedule building");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	complete_phase(outputStreamHdl, phasePtr);
	
	// 4) Building the linearized schedule
	new_phase(outputStreamHdl, phasePtr);
	
	outcome = eliminate_parameters(outputStreamHdl, polyhedralModelPtr, modifiedPolyhedralModel, numTasks);
	
	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during parameter projection out");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	outcome = linearize_dates(outputStreamHdl, modifiedPolyhedralModel, numTasks);
	
	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during dates linearization");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	complete_phase(outputStreamHdl, phasePtr);
	
	// Be clean
	manipulated_polyhedral_model_array_free(modifiedPolyhedralModel, numTasks);
	free(tasks);
	finish(outputStreamHdl, phasePtr);
}

char ** validate_input(int n, char ** argv) {
	
	char ** names = malloc(n * sizeof(char *));
	
	for (int i = 0; i < n; i++)
		names[i] = argv[i+OPTIONS+1];
	
	return names;
}