#ifdef MOREVERBOSE
#define VERBOSE
#endif
/*
 * Implementation of the Lattice - Based Memory Mapping Banks Mapping techniquq
 * for the Uniform Memory Access time case
 */ 
#include<stdlib.h>
#include<string.h>

#ifdef VERBOSE
#include<stdio.h>
#endif

#include<pet.h>

#include "support.h"
#include "partitioning.h"

#define DIMSTRING 100

const char * sourceRelativePath = "../Tests/source-demos/";
const char * sourceExtension = ".c";
const char * scheduleRelativePath = "../Tests/polyhedral-extraction/outputs/"; 
const char * scheduleExtension = ".isl.schedule";

char ** validate_input(int, char**);

// Note that when an array lasts in Ptr, its elements are pointers
int main(int argc, char ** argv) {
	// Pointer to the current phase, used for graphical purposes
	phase * phasePtr = NULL;
		// Handle for the configuration of the isl and pet libraries
	isl_ctx * optionsHdl = NULL;
	// Array of task names
	char ** tasks = NULL;
	// File name with relative path
	char * fileName;
	// Handle to the file containing the modified schedule
	FILE * filePtr;
	// Total numbers of tasks to work with
	unsigned numTasks = 0;
	// Array of polyhedral models of each task
	pet_scop ** polyhedralModelPtr = NULL;
	// Array containing the access relations aftew virtual address space allocation
	remapped_access_relations * remappedAccessRelations = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	
	// 1a) We check if the user passed some sources to work with 
	phasePtr = start();
	
	if (phasePtr == NULL) {
		error("Memory allocation problem :(", phasePtr);
	}
	
	if (argc <= 1) {
		error("Not enough input file(s)", phasePtr);
	}
	
	numTasks = argc - 1;
	
#ifdef VERBOSE
	printf("Overall number of tasks: %d\n", numTasks);
#endif
	
	tasks = validate_input(numTasks, argv);
	
	if (tasks == NULL)
		abort_phase(phasePtr);
	
#ifdef VERBOSE
	printf("Task names:\n");
	for (int i = 0; i < numTasks; i++)
		printf("%d)\t%s\n", i, tasks[i]);
#endif
	
	// 1b) Now we parse the input sources to get the whole polyhedral model
	optionsHdl = isl_ctx_alloc_with_pet_options();
	
	if (optionsHdl == NULL) 
		error("Sorry, there is something wrong with the pet library :(", phasePtr);
	
	polyhedralModelPtr = malloc(numTasks * sizeof(pet_scop *));
	
	for (int i = 0; i < numTasks; i++) {
		fileName = malloc(DIMSTRING * sizeof(char));
		fileName[0] = '\0';
		
		strcat(fileName, sourceRelativePath);
		strcat(fileName, tasks[i]);
		strcat(fileName, sourceExtension);
		
#ifdef VERBOSE
		printf("Parsing file %s\n", fileName);
#endif
		
		polyhedralModelPtr[i] = pet_scop_extract_from_C_source(optionsHdl, fileName, NULL);
		
		if (polyhedralModelPtr[i] == NULL)
			abort_phase(phasePtr);
		
#ifdef MOREVERBOSE
		printf("Polyhedral model:\n");
		fflush(stdout);
		pet_scop_dump(polyhedralModelPtr[i]);
#endif
		
		// We replace the parsed schedule with the one read from the modified schedule file
		free(fileName);
		
		fileName = malloc(DIMSTRING * sizeof(char));
		fileName[0] = '\0';
		
		strcat(fileName, scheduleRelativePath);
		strcat(fileName, tasks[i]);
		strcat(fileName, scheduleExtension);
		
#ifdef VERBOSE
		printf("Reading file %s\n", fileName);
#endif
		filePtr = fopen(fileName, "r");
		
		if (filePtr == NULL)
			abort_phase(phasePtr);
		
		polyhedralModelPtr[i] -> schedule = isl_schedule_read_from_file(optionsHdl, filePtr);
		
#ifdef MOREVERBOSE
		printf("Modified polyhedral model:\n");
		fflush(stdout);
		pet_scop_dump(polyhedralModelPtr[i]);
#endif
		
		// Be clean for the next file
		fclose(filePtr);
		free(fileName);
	}
	
	complete_phase(phasePtr);
	
	// 2) Virtual memory allocation 
	new_phase(phasePtr);
	
	remappedAccessRelations = malloc(numTasks * sizeof(remappedAccessRelations));
	
	if (remappedAccessRelations == NULL)
		abort_phase(phasePtr);
	
	outcome = virtual_allocation(optionsHdl, polyhedralModelPtr, remappedAccessRelations, numTasks);
	
	if (outcome == isl_stat_error)
		abort_phase(phasePtr);
	
	complete_phase(phasePtr);
	
	// Be clean
	free(remappedAccessRelations);
	free(tasks);
	finish(phasePtr);
}

char ** validate_input(int n, char ** argv) {
	
	char ** names = malloc(n * sizeof(char *));
	
	for (int i = 0; i < n; i++)
		names[i] = argv[i+1];
	
	return names;
}