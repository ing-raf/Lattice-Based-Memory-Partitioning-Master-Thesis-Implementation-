#ifdef MOREVERBOSE
#define VERBOSE
#endif
/*
 * Implementation of functions used to build the polyhedral model from the input
 */
#include<stdlib.h>
#include<string.h>

#include "config.h"
#include "support.h"
#include "partitioning.h"

#define DIMSTRING 100

const char * sourceRelativePath = "../Tests/source-demos/";
const char * sourceExtension = ".c";
const char * scheduleRelativePath = "../Tests/polyhedral-extraction/outputs/"; 
const char * scheduleExtension = ".isl.schedule";
const char * latticesRelativePath = "./Lattices/";
const char * numLatticesSuffix = "numLattices";
const char * latticesExtension = ".txt";
const char * latticesFormat = "%s%i_dim%i_lattice%i_translate%i%s";

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

isl_set *** parse_lattices (FILE * stream, isl_ctx * optionsHdl, unsigned * numLatticesPtr, unsigned dim) {
	// Array of the lattices
	isl_set *** translatesPtr = NULL;
	// Handle to the file containing a lattice
	FILE * latticeHdl = NULL;
	// String for the file name of the lattice
	char * latticeFileName = NULL;
	#ifdef MOREVERBOSE
		// Pointer to the printer
		isl_printer * printer = NULL;
	#endif
	
	// First of all, we read the number of different fundamental lattices
	latticeFileName = malloc (DIMSTRING * sizeof(char));
	
	if (latticeFileName == NULL) {
		error(stream, "Memory allocation problem :(");
		return NULL;
	}
	
	if (sprintf(latticeFileName, "%s%i_dim%i_%s%s", latticesRelativePath, NUMBANKS, dim, numLatticesSuffix, latticesExtension) < 0) {
		error(stream, "Problem when building the name of the file containing the number of lattices");
		return NULL;
	}
	
	latticeHdl = fopen(latticeFileName, "r");
	
	if (latticeHdl == NULL) {
		error(stream, "File not found");
		return NULL;
	}
	
	fscanf(latticeHdl, "Number of different fundamental lattices: ");
	fscanf(latticeHdl, "%i", numLatticesPtr);
	
	#ifdef VERBOSE
		fprintf(stream, "Number of different fundamental lattices: %i\n", *numLatticesPtr);
		fflush(stream);
	#endif
	
	fclose(latticeHdl);
	free(latticeFileName);
	
	// Here we exploit the fact that for each fundamental lattice there are exactly NUMBANK translates
//	translatesPtr = malloc ((*numLatticesPtr * NUMBANKS) * sizeof (isl_set *) );
	
	translatesPtr = malloc ((*numLatticesPtr) * sizeof (isl_set **) );
	
	if (translatesPtr == NULL) {
		error(stream, "Memory allocation problem :(");
		return NULL;
	}
	
	// Now we read all the translates for all the lattices
	for (int i = 1; i <= *numLatticesPtr; i++) {
		translatesPtr[i-1] = malloc (NUMBANKS * sizeof(isl_set *));
		
		
		if (translatesPtr[i-1] == NULL) {
			error(stream, "Memory allocation problem :(");
			return NULL;
		}
		
		for (int j = 1; j <= NUMBANKS; j++) {
			latticeFileName = malloc (DIMSTRING * sizeof(char));
			
			if (latticeFileName == NULL) {
				error(stream, "Memory allocation problem :(");
				return NULL;
			}
			
			if (sprintf(latticeFileName, latticesFormat, 
				latticesRelativePath, NUMBANKS,dim,  i, j, latticesExtension) < 0) {
				error(stream, "Problem when building the name of the file containing the number of lattices");
				return NULL;
			}
			
			latticeHdl = fopen(latticeFileName, "r");
			
			if (latticeHdl == NULL) {
				error(stream, "File not found");
				return NULL;
			}
			// Be careful of off - by - one errors!!!!!!
			translatesPtr[i-1][j-1] = isl_set_read_from_file(optionsHdl, latticeHdl);
			
			if (translatesPtr[i-1][j-1] == NULL) {
				error(stream, "Problems when reading a lattice");
				return NULL;
			}
			
			#ifdef MOREVERBOSE
				fprintf(stream, "Translate %i of lattice %i:\n", j, i);
				fflush(stream);
				printer = isl_printer_to_file(optionsHdl, stream);
				
				if(printer == NULL) {
					error(stream, "Memory allocation problem :(");
					return NULL;
				} 
				
				printer = isl_printer_print_set(printer, translatesPtr[i-1][j-1]);
				
				if(printer == NULL) {
					error(stream, "Printing problem :(");
					return NULL;
				}
				
				fprintf(stream, "\n");
				fflush(stream);
			#endif
			
			fclose(latticeHdl);
			free(latticeFileName);
		}
		
	}
	#ifdef MOREVERBOSE
		isl_printer_free(printer);
	#endif
	return translatesPtr;
}


