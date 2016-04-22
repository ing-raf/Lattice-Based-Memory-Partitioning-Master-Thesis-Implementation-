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

typedef struct {
	phase * phasePtr;
	FILE * stream;
	unsigned numTasks;
	manipulated_polyhedral_model ** modifiedPolyhedralModelPtr;
} concurrent_part_params;

char ** validate_input(int, char**);
isl_stat concurrent_part(isl_point *, void *);

// Note that when an array lasts in Ptr, its elements are pointers
int main(int argc, char ** argv) {
	// Pointer to the current phase, used for graphical purposes
	phase * phasePtr = NULL;
	// Handle to the output stream
	FILE * outputStreamHdl = NULL;
	// Handle for the configuration of the isl and pet libraries
	isl_ctx * optionsHdl = NULL;
#ifdef VERBOSE
	// Pointer to the printer
	isl_printer * printer = NULL;
#endif
	// Array of task names
	char ** tasks = NULL;
	// Total numbers of tasks to work with
	unsigned numTasks = 0;
	// Array of the original polyhedral models of each task
	pet_scop ** polyhedralModelPtr = NULL;
	// Array containing the manipulated version of the polyhedral models of each task
	manipulated_polyhedral_model ** modifiedPolyhedralModelPtr = NULL;
	// Set of all the linearized dates across the concurrent tasks
	isl_set * linearized_schedule_dates_set = NULL;
	// Parameters for the callback function
	concurrent_part_params * params = NULL;
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
	
	modifiedPolyhedralModelPtr = manipulated_polyhedral_model_array_alloc(numTasks);
	
	if (modifiedPolyhedralModelPtr == NULL) {
		error(outputStreamHdl, "Memory allocation problem :(");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	outcome = virtual_allocation(outputStreamHdl, optionsHdl, polyhedralModelPtr, modifiedPolyhedralModelPtr, numTasks);
	
	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during virtual address space allocation");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	complete_phase(outputStreamHdl, phasePtr);
	
	// 3) Building the physical schedule
	new_phase(outputStreamHdl, phasePtr);
	
	outcome = physical_schedule(outputStreamHdl, optionsHdl, polyhedralModelPtr, modifiedPolyhedralModelPtr, numTasks);
	
	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during physical schedule building");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	complete_phase(outputStreamHdl, phasePtr);
	
	// 4) Building the linearized schedule
	new_phase(outputStreamHdl, phasePtr);
	
	outcome = eliminate_parameters(outputStreamHdl, polyhedralModelPtr, modifiedPolyhedralModelPtr, numTasks);
	
	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during parameter projection out");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	outcome = linearize_dates(outputStreamHdl, modifiedPolyhedralModelPtr, numTasks);
	
	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during dates linearization");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	linearized_schedule_dates_set = isl_set_from_union_set(isl_union_map_range(isl_union_map_copy(modifiedPolyhedralModelPtr[0] -> linearizedSchedule)));
	
	if (linearized_schedule_dates_set == NULL) {
		error(outputStreamHdl, "Error during dates union");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	for (int i = 1; i < numTasks; i++)
		linearized_schedule_dates_set = isl_set_union(linearized_schedule_dates_set, isl_set_from_union_set(isl_union_map_range(isl_union_map_copy(modifiedPolyhedralModelPtr[i] -> linearizedSchedule))));
	
	linearized_schedule_dates_set = isl_set_coalesce(linearized_schedule_dates_set);
	
	if (linearized_schedule_dates_set == NULL) {
		error(outputStreamHdl, "Error during dates union");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
#ifdef VERBOSE
	fprintf(outputStreamHdl, "Unified linearized schedule space:\n");
	fflush(outputStreamHdl);
	printer = isl_printer_to_file(optionsHdl, outputStreamHdl);
	
	if(printer == NULL) {
		error(outputStreamHdl, "Memory allocation problem :(");
		abort_phase(outputStreamHdl, phasePtr);
	} 
	
	printer = isl_printer_print_set(printer, linearized_schedule_dates_set);
	
	if(printer == NULL) {
		error(outputStreamHdl, "Printing problem :(");
		abort_phase(outputStreamHdl, phasePtr);
	} 
	
	fprintf(outputStreamHdl, "\n");
	
//	isl_printer_free(printer);
#endif
	
	complete_phase(outputStreamHdl, phasePtr);
	
	// This part must be iterated for each one of the linearized dates
	params = malloc(sizeof(concurrent_part_params));
	
	if (params == NULL) {
		error(outputStreamHdl, "Memory allocation problem for the parameters of the concurrent part:(");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	params -> phasePtr = phasePtr;
	params -> stream = outputStreamHdl;
	params -> numTasks = numTasks;
	params -> modifiedPolyhedralModelPtr = modifiedPolyhedralModelPtr;
	
	outcome = isl_set_foreach_point(linearized_schedule_dates_set, concurrent_part, (void *)params);
	
	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during the concurrent part");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	// Be clean
//	free(params);
	manipulated_polyhedral_model_array_free(modifiedPolyhedralModelPtr, numTasks);
	free(tasks);
	finish(outputStreamHdl, phasePtr);
}

char ** validate_input(int n, char ** argv) {
	
	char ** names = malloc(n * sizeof(char *));
	
	for (int i = 0; i < n; i++)
		names[i] = argv[i+OPTIONS+1];
	
	return names;
}

isl_stat concurrent_part(isl_point * pointPtr, void * user) {
	// Pointer to the input parameters
	concurrent_part_params * params = (concurrent_part_params *)user;
	// Pointer to the printer
	isl_printer * printer = NULL;
	// Pointer to the phase of the current point
	phase phasePoint = *params -> phasePtr;
	// Pointer to the array of the polyhedral slices
	isl_union_set ** polyhedralSlicePtr = NULL;
	// Pointer to the concurrent dataset
	isl_set * concurrentDatasetPtr = NULL;
	
	info (params -> stream, "Linearized date: %d", isl_val_get_num_si(isl_point_get_coordinate_val(pointPtr, isl_dim_set, 0)));
	
	printer = isl_printer_to_file(isl_point_get_ctx(pointPtr), params -> stream);
	
	// 5) Polyhedral slices building
	new_phase(params -> stream, &(phasePoint));
	
	polyhedralSlicePtr = malloc ((params -> numTasks) * sizeof(isl_union_set *));
	
	if (polyhedralSlicePtr == NULL) {
		error(params -> stream, "Memory allocation problem for the polyhedral slices");
		return isl_stat_error;
	}
	
	for (int i = 0; i < params -> numTasks; i++) {
		
#ifdef MOREVERBOSE
		info(params -> stream, "Task %d)", i);
#endif
		
		polyhedralSlicePtr[i] = polyhedral_slice_build (params -> stream, isl_union_map_copy(params -> modifiedPolyhedralModelPtr[i] -> flattenedSchedule), isl_union_map_copy(params -> modifiedPolyhedralModelPtr[i] -> linearizedSchedule), pointPtr);
		
		
		if (polyhedralSlicePtr[i] == NULL) {
			error(params -> stream, "Error during polyhedral slices building");
			return isl_stat_error;
		}
		
#ifdef MOREVERBOSE
		printer = isl_printer_set_indent(printer, moreIndent);
		fprintf(params -> stream, "Polyhedral slice of task %d:\n", i);
		printer = isl_printer_print_union_set(printer, polyhedralSlicePtr[i]);
		
		if(printer == NULL) {
			error(params -> stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(params -> stream, "\n");
		fflush(params -> stream);
#endif
	}
		
	complete_phase(params -> stream, &(phasePoint));
	
	// 6) Concurrent dataset building
	new_phase(params -> stream, &(phasePoint));
	
	concurrentDatasetPtr = concurrent_dataset_build(params -> stream, params -> modifiedPolyhedralModelPtr, polyhedralSlicePtr, params -> numTasks);
	
	if (concurrentDatasetPtr == NULL) {
		error(params -> stream, "Error during concurrent dataset building");
		return isl_stat_error;
	}
	
#ifdef VERBOSE
	fprintf(params -> stream, "Concurrent dataset:\n");
	printer = isl_printer_print_set(printer, concurrentDatasetPtr);
		
	if(printer == NULL) {
		error(params -> stream, "Printing problem :(");
		return isl_stat_error;
	} 
	
	fprintf(params -> stream, "\n");
	fflush(params -> stream);
#endif
	
	complete_phase(params -> stream, &(phasePoint));
	
	isl_printer_free(printer);
	
	return isl_stat_ok;
}