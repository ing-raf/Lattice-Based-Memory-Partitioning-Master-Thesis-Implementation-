#ifdef MOREVERBOSE
#define VERBOSE
#endif
/*
 * Implementation of functions used to build the polyhedral model from the input
 */
#include<stdlib.h>
#include<string.h>

#include "support.h"
#include "partitioning.h"

#define DIMSTRING 100

const char * sourceRelativePath = "../Tests/source-demos/";
const char * sourceExtension = ".c";
const char * scheduleRelativePath = "../Tests/polyhedral-extraction/outputs/"; 
const char * scheduleExtension = ".isl.schedule";

isl_stat parse_input(FILE * stream, isl_ctx * optionsHdl, char ** tasks, pet_scop ** polyhedralModelPtr, unsigned numTasks) {
	// File name with relative path
	char * fileName;
	// Handle to the file containing the modified schedule
	FILE * filePtr;
	
	for (int i = 0; i < numTasks; i++) {
		fileName = malloc(DIMSTRING * sizeof(char));
		fileName[0] = '\0';
		
		strcat(fileName, sourceRelativePath);
		strcat(fileName, tasks[i]);
		strcat(fileName, sourceExtension);
		
#ifdef VERBOSE
		fprintf(stream, "Parsing file %s\n", fileName);
#endif
		
		polyhedralModelPtr[i] = pet_scop_extract_from_C_source(optionsHdl, fileName, NULL);
		
		if (polyhedralModelPtr[i] == NULL) {
			error(stream, "Sorry, there is something wrong with the pet library :(");
			return isl_stat_error;
		}
		
#ifdef MOREVERBOSE
		fprintf(stream, "Polyhedral model:\n");
		fflush(stream);
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
		fprintf(stream, "Reading file %s\n", fileName);
#endif
		filePtr = fopen(fileName, "r");
		
		if (filePtr == NULL) {
			error(stream, "File not found");
			return isl_stat_error;
		}
		
		polyhedralModelPtr[i] -> schedule = isl_schedule_read_from_file(optionsHdl, filePtr);
		
#ifdef MOREVERBOSE
		fprintf(stream, "Modified polyhedral model:\n");
		fflush(stream);
		pet_scop_dump(polyhedralModelPtr[i]);
#endif
		
		// Be clean for the next file
		fclose(filePtr);
		free(fileName);
	}
	
	return isl_stat_ok;
}



