#ifdef MOREVERBOSE
	#define VERBOSE
#endif
/*
 * Implementation of the Lattice - Based Memory Mapping Banks Mapping techniquq
 * for the Non - Uniform Memory Access time case
 */ 
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#include<pet.h>

#include "config.h"
#include "support.h"
#include "model.h"
#include "partitioning.h"

#define REDUCED_LATTICES 60
#define DIMSTRING 100

#define OUTPUT_FILE argv[1]
#define ARCHITECTURE_FILE argv[2]

const unsigned config_input = 2;
const unsigned parallel_phases = 2;

typedef struct {
	phase * phasePtr;
	FILE * stream;
	unsigned numProcessors;
	unsigned numTranslates;
	manipulated_polyhedral_model ** modifiedPolyhedralModelPtr;
	isl_set *** translatesPtr;
	dataset_type_array ** datasetTypesPtr;
	unsigned numLattices;
} concurrent_part_params;

char ** parse_task_names(int, char**);
isl_stat concurrent_part(isl_point *, void *);

// Note that when an array lasts in Ptr, its elements are pointers
int main(int argc, char ** argv) {
	// Pointer to the current phase, used for graphical purposes
	phase * phasePtr = NULL;
	// Handle to the output stream
	FILE * outputStreamHdl = NULL;
	// Number of processors in the architecture
	unsigned numProcessors = 0;
	// Number of memory banks in the architecture
	unsigned numBanks = 0;
	// Array of service latencies of each memory bank
	unsigned * bankLatency = NULL;
	// Array of delay incurred by a specific processor accessing a specific memory bank
	unsigned ** processorToBankDelay = NULL;
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
	// Array of the lattices
	isl_set *** translatesPtr = NULL;
	// Number of different fundamental lattices
	unsigned numLattices = 0;
	// Array containing the manipulated version of the polyhedral models of each task
	manipulated_polyhedral_model ** modifiedPolyhedralModelPtr = NULL;
	// Dimensionality of the address space
	unsigned dimAddressSpace = 0;
	// Set of all the linearized dates across the concurrent tasks
	isl_set * linearized_schedule_dates_set = NULL;
	// Array of access matrices for each fundamental lattice
	dataset_type_array ** datasetTypesPtr = NULL;
	// Index of the best fundamental lattice
	unsigned bestLatticeIdx = 0;
	// String with the result of the computation
	char * result = NULL;
	// Parameters for the callback function
	concurrent_part_params * params = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	
	// 1a) We check if the user passed some sources to work with 
	if (argc <= config_input + 1) {
		perror("Not enough input file(s)");
		exit(1);
	}
	
	if (strcmp(argv[1], "stdout") == 0)
		outputStreamHdl = stdout;
	else {
		outputStreamHdl = fopen(OUTPUT_FILE,"w");
		
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

	outcome = parse_architecture(outputStreamHdl, ARCHITECTURE_FILE, &numProcessors, &numBanks, &bankLatency, &processorToBankDelay);

	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during parsing the architecture file");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	numTasks = argc - config_input - 1;
	
	tasks = parse_task_names(numTasks, argv);
	
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
	
	outcome = virtual_allocation(outputStreamHdl, optionsHdl, polyhedralModelPtr, modifiedPolyhedralModelPtr, numTasks, &dimAddressSpace);
	
	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during virtual address space allocation");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	// 3) Reading the lattices with all the translates
	new_phase(outputStreamHdl, phasePtr);
	
	translatesPtr = parse_lattices(outputStreamHdl, optionsHdl, &numLattices, numBanks, dimAddressSpace);
	
	if (translatesPtr == NULL) {
		error(outputStreamHdl, "Error during parsing lattices :(");
		abort_phase(outputStreamHdl, phasePtr);
	}

	#ifdef NOLATTICES
		numLattices = REDUCED_LATTICES;
	#endif
	
	complete_phase(outputStreamHdl, phasePtr);
	
	// 4) Building the physical schedule
	new_phase(outputStreamHdl, phasePtr);
	
	outcome = physical_schedule(outputStreamHdl, optionsHdl, polyhedralModelPtr, modifiedPolyhedralModelPtr, numTasks);
	
	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during physical schedule building");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	complete_phase(outputStreamHdl, phasePtr);

	// 5) Building the allocation constraint
	new_phase(outputStreamHdl, phasePtr);
	
	outcome = allocation_constraint(outputStreamHdl, optionsHdl, modifiedPolyhedralModelPtr, numTasks);
	
	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during allocation constraint building");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	complete_phase(outputStreamHdl, phasePtr);
	
	// 6) Building the linearized schedule
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
		
		isl_printer_free(printer);
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
	params -> numProcessors = numProcessors;
	params -> numTranslates = numBanks;
	params -> modifiedPolyhedralModelPtr = modifiedPolyhedralModelPtr;
	params -> translatesPtr = translatesPtr;
	params -> numLattices = numLattices;
	params -> datasetTypesPtr = malloc(numLattices * sizeof(dataset_type_array *));
	
	if(params -> datasetTypesPtr == NULL) {
		error(outputStreamHdl, "Memory allocation problem :(");
		abort_phase(outputStreamHdl, phasePtr);
	} 
	
	for (int i = 0; i < numLattices; i++)
		params -> datasetTypesPtr[i] = dataset_type_array_alloc(numBanks, numProcessors);
	
	outcome = isl_set_foreach_point(linearized_schedule_dates_set, concurrent_part, (void *)params);
	
	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during the concurrent part");
		phasePtr -> phase_num += parallel_phases;
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	phasePtr -> phase_num += parallel_phases;

	// 9) Cost function evaluation
	new_phase(outputStreamHdl, phasePtr);
	
	datasetTypesPtr = params -> datasetTypesPtr;

	#ifdef MOREVERBOSE
		for (unsigned i = 0; i < numLattices; i++) {
			info(outputStreamHdl,"Fundamental lattice %i)", i);

			dataset_type_array_fprintf(outputStreamHdl, datasetTypesPtr[i]);
		}
	#endif

	warning(outputStreamHdl, "Different bank latencies are currently unsupported. Latencies other than the first bank will be ignored");

	outcome = MILPsolve(outputStreamHdl, numBanks, datasetTypesPtr, numLattices, bankLatency[0], &bestLatticeIdx, processorToBankDelay);

	if (outcome == isl_stat_error) {
		error(outputStreamHdl, "Error during the solving of the MILP model");
		abort_phase(outputStreamHdl, phasePtr);
	}
	
	complete_phase(outputStreamHdl, phasePtr);
	
	result = (char *)malloc(DIMSTRING * sizeof(char));

	if (result == NULL) {
		error(outputStreamHdl, "Memory allocation problem with the result string :(");
		abort_phase(outputStreamHdl, phasePtr);
	}

	result[0] = '\0';

	if (sprintf(result, "The best allocation is the one corresponding to the lattice number %u", bestLatticeIdx) < 0) {
		error(outputStreamHdl, "Problem during writing the result string :(");
		abort_phase(outputStreamHdl, phasePtr);
	}

	news(outputStreamHdl, result);
	
	// Be clean
	free(result);
	for(unsigned i = 0; i <numLattices; i++)
		dataset_type_array_free(datasetTypesPtr[i]);
	free(datasetTypesPtr);
	free(params);
	manipulated_polyhedral_model_array_free(modifiedPolyhedralModelPtr, numTasks);
	free(tasks);
	finish(outputStreamHdl, phasePtr);
}

char ** parse_task_names(int n, char ** argv) {
	
	char ** names = malloc(n * sizeof(char *));
	
	for (int i = 0; i < n; i++)
		names[i] = argv[i+config_input+1];
	
	return names;
}

isl_stat concurrent_part(isl_point * pointPtr, void * user) {
	// Pointer to the input parameters
	concurrent_part_params * params = (concurrent_part_params *)user;
	// Pointer to the printer
	isl_printer * printer = NULL;
	// Pointer to the phase of the current point
	phase phasePoint = *params -> phasePtr;
	// Pointer to the array of the instant local slices
	isl_union_set ** instantLocalSlicePtr = NULL;
	// Pointer to the array of the instant local dataset
	isl_set ** instantLocalDatasetPtr = NULL;
	// Pointer to the access matrix
	unsigned ** accessMatrix = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	
	info (params -> stream, "Linearized date: %d", isl_val_get_num_si(isl_point_get_coordinate_val(pointPtr, isl_dim_set, 0)));
	
	printer = isl_printer_to_file(isl_point_get_ctx(pointPtr), params -> stream);
	
	// 7) Instant local slices building
	new_phase(params -> stream, &(phasePoint));
	
	instantLocalSlicePtr = malloc ((params -> numProcessors) * sizeof(isl_union_set *));
	
	if (instantLocalSlicePtr == NULL) {
		error(params -> stream, "Memory allocation problem for the instant local slices");
		return isl_stat_error;
	}
	
	instantLocalDatasetPtr = malloc ((params -> numProcessors) * sizeof(isl_set *));
	
	if (instantLocalDatasetPtr == NULL) {
		error(params -> stream, "Memory allocation problem for the instant local datasets");
		return isl_stat_error;
	}
	
	for (int i = 0; i < params -> numProcessors; i++) {
		
		#ifdef MOREVERBOSE
			info(params -> stream, "Processor %d)", i);
		#endif
		
		instantLocalSlicePtr[i] = instant_local_slice_build (
			params -> stream, 
			params -> modifiedPolyhedralModelPtr[TASK[i]] -> instanceSet,
			params -> modifiedPolyhedralModelPtr[TASK[i]] -> flattenedSchedule, 
			params -> modifiedPolyhedralModelPtr[TASK[i]] -> linearizedSchedule,
			params -> modifiedPolyhedralModelPtr[TASK[i]] -> allocation,  
			pointPtr, 
			i);
		
		if (instantLocalSlicePtr[i] == NULL) {
			error(params -> stream, "Error during instant local slices building");
			return isl_stat_error;
		}
		
		#ifdef VERBOSE
			printer = isl_printer_set_indent(printer, moreIndent);
			fprintf(params -> stream, "Instant local slice of processor %d:\n", i);
			printer = isl_printer_print_union_set(printer, instantLocalSlicePtr[i]);
			
			if(printer == NULL) {
				error(params -> stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(params -> stream, "\n");
			fflush(params -> stream);
		#endif
	}
		
	instantLocalDatasetPtr = instant_local_dataset_build(params -> stream, params -> modifiedPolyhedralModelPtr, instantLocalSlicePtr, params -> numProcessors);
		
	if (instantLocalDatasetPtr == NULL) {
		error(params -> stream, "Error during instant local dataset building");
		return isl_stat_error;
	}
	
	complete_phase(params -> stream, &(phasePoint));
	
	// 8) Computation of the parameters needed by the mapping algorithm for the current date
	new_phase(params -> stream, &(phasePoint));
	
	for (unsigned i = 0; i < params -> numLattices; i++) {
		#ifdef MOREVERBOSE
			info(params -> stream, "Fundamental lattice %u)", i);
		#endif
		
		accessMatrix = (unsigned **)malloc((params -> numTranslates) * sizeof(unsigned *));
		
		if (accessMatrix == NULL) {
			error(params -> stream, "Memory allocation problem for the access matrix");
			return isl_stat_error;
		}

		for (unsigned j = 0; j < params -> numTranslates; j++) {
			// Row - stored matrix. The index i has already been used for the fundamental lattice
			accessMatrix[j] = (unsigned *)malloc((params -> numProcessors) * sizeof(unsigned));

			if (accessMatrix[j] == NULL) {
				error(params -> stream, "Memory allocation problem for the access matrix");
				return isl_stat_error;
			}

		}
		
		outcome = access_matrix_fundamental_lattice(params -> stream, instantLocalDatasetPtr, params -> translatesPtr[i], params -> numTranslates, params -> numProcessors, accessMatrix);
		
		if (outcome == isl_stat_error) {
			error(params -> stream, "Error during the evaluation of the cost function");
			return isl_stat_error;
		} 
		
		outcome = dataset_type_array_add(params -> datasetTypesPtr[i], accessMatrix);
		
		if (outcome == isl_stat_error) {
			error(params -> stream, "Error during dataset types evaluation");
			return isl_stat_error;
		} 
	}
	
	complete_phase(params -> stream, &(phasePoint));
	
	isl_printer_free(printer);
	
	return isl_stat_ok;
}