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


const char * architectureRelativePath = "./Architectures/";
const char * architectureExtension = ".txt";
const char * sourceRelativePath = "../Tests/source-demos/";
const char * sourceExtension = ".c";
const char * scheduleRelativePath = "../Tests/polyhedral-extraction/outputs/"; 
const char * scheduleExtension = ".isl.schedule";
const char * latticeRelativePath = "./Lattices/";
const char * numLatticesSuffix = "numLattices";
const char * latticeExtension = ".txt";
const char * latticeFormat = "%s%i_dim%i_lattice%i_translate%i%s";

/**
 * Function that reads and parses the file containing the specified architecture
 * @param  stream                  Stream for the output messages
 * @param  name                    Name of the file containing the desctiption of the architecture
 * @param  numProcessorsPtr        (output) Number of processors in the architecture
 * @param  numBanksPtr             (output) Number of memory banks in the architecture
 * @param  bankLatencyPtr          (output) Array of the service latency of each memory bank
 * @param  processorToBankDelayPtr (output) Array of the delay incurred by a specified processor to access a specified memory bank
 * @return                         isl_stat_ok if no problem occurs, isl_stat_error otherwise
 */
isl_stat parse_architecture (FILE * stream, char * name, unsigned * numProcessorsPtr, unsigned * numBanksPtr, unsigned ** bankLatencyPtr, unsigned *** processorToBankDelayPtr) {
	// Name of the file containing the description of the architecture with relative path
	char * fileName = NULL;
	// Handle to the file containing the description of the architecture
	FILE * fileHdl = NULL;
	// Array of bank latencies
	unsigned * bankLatency = NULL;
	// Array of access latencies from a processor to a bank
	unsigned ** processorToBankDelay = NULL;

	fileName = (char *)malloc(DIMSTRING * sizeof(char));

	if (fileName == NULL) {
		error(stream, "Memory allocation problem with the architecture file name :(");
		return isl_stat_error;
	}

	fileName[0] = '\0';

	if (sprintf(fileName, "%s%s%s", architectureRelativePath, name, architectureExtension) < 0) {
		error(stream, "Problem when building the name of the file containing description of the architecture");
		return isl_stat_error;
	}

	fileHdl = fopen(fileName, "r");

	if (fileHdl == NULL) {
		error(stream, "File not found");
		return isl_stat_error;
	}
	
	// Reading the number of processors
	fscanf(fileHdl, "Number of processors: %u ", numProcessorsPtr);
	
	fprintf(stream, "Number of processors in the architecture: %u\n", *numProcessorsPtr);

	// Reading the number of memory banks
	fscanf(fileHdl, "Number of memory banks: %u ", numBanksPtr);
	
	fprintf(stream, "Number of memory banks in the architecture: %u\n", *numBanksPtr);

	// Reading the memrory bank latencies
	bankLatency = (unsigned *)malloc((*numBanksPtr) * sizeof(unsigned));

	if (bankLatency == NULL) {
		error(stream, "Memory allocation problem with the bank latencies array :(");
		return isl_stat_error;
	}

	fscanf(fileHdl, "Latency of each memory bank: ");

	for(unsigned i = 0; i < *numBanksPtr; i++)
		fscanf(fileHdl, "%u ", &(bankLatency[i]));

	fprintf(stream, "Memory banks #\t Latency\n");

	for(unsigned i = 0; i < *numBanksPtr; i++)
		fprintf(stream, "%u\t\t\t%u\n", i, bankLatency[i]);

	// Reading the access latencies
	processorToBankDelay = (unsigned **)malloc((*numProcessorsPtr) * sizeof(unsigned *));

	if (processorToBankDelay == NULL) {
		error(stream, "Memory allocation problem with the bank latencies array :(");
		return isl_stat_error;
	}

	for(unsigned i = 0; i < *numProcessorsPtr; i++) {
		processorToBankDelay[i] = (unsigned *)malloc((*numBanksPtr) * sizeof(unsigned));

		if (processorToBankDelay[i] == NULL) {
			error(stream, "Memory allocation problem with the bank latencies array :(");
			return isl_stat_error;
		}	

	}

	fscanf(fileHdl, "Latency from each processor to each memory bank: ");

	for(unsigned i = 0; i < *numProcessorsPtr; i++) 
		for(unsigned j = 0; j < *numBanksPtr; j++)
			fscanf(fileHdl, "%u ", &(processorToBankDelay[i][j]));

	fprintf(stream, "Processor #\t Memory banks #\t Latency\n");

	for(unsigned i = 0; i < *numProcessorsPtr; i++) 
		for(unsigned j = 0; j < *numBanksPtr; j++)
			fprintf(stream, "%u\t\t\t%u\t\t\t%u ", i, j, processorToBankDelay[i][j]);

	// Be clean
	fclose(fileHdl);
	free(fileName);

	*processorToBankDelayPtr = processorToBankDelay;
	*bankLatencyPtr = bankLatency;

	return isl_stat_ok;
}

isl_stat parse_input(FILE * stream, isl_ctx * optionsHdl, char ** tasks, pet_scop ** polyhedralModelPtr, unsigned numTasks) {
	// File name with relative path
	char * fileName;
	// Handle to the file containing the modified schedule
	FILE * fileHdl;
	
	for (int i = 0; i < numTasks; i++) {
		fileName = (char *)malloc(DIMSTRING * sizeof(char));

		if (fileName == NULL) {
			error(stream, "Memory allocation problem with the source file name :(");
			return isl_stat_error;
		}

		fileName[0] = '\0';
		
		if (sprintf(fileName, "%s%s%s", sourceRelativePath, tasks[i], sourceExtension) < 0) {
			error(stream, "Problem when building the source file name");
			return isl_stat_error;
		}

		// strcat(fileName, sourceRelativePath);
		// strcat(fileName, tasks[i]);
		// strcat(fileName, sourceExtension);
		
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

		if (fileName == NULL) {
			error(stream, "Memory allocation problem with the schedule file name :(");
			return isl_stat_error;
		}

		fileName[0] = '\0';

		if (sprintf(fileName, "%s%s%s", scheduleRelativePath, tasks[i], scheduleExtension) < 0) {
			error(stream, "Problem when building the schedule file name");
			return isl_stat_error;
		}
		
		// strcat(fileName, scheduleRelativePath);
		// strcat(fileName, tasks[i]);
		// strcat(fileName, scheduleExtension);
		
		#ifdef VERBOSE
			fprintf(stream, "Reading file %s\n", fileName);
		#endif

		fileHdl = fopen(fileName, "r");
		
		if (fileHdl == NULL) {
			error(stream, "File not found");
			return isl_stat_error;
		}
		
		polyhedralModelPtr[i] -> schedule = isl_schedule_read_from_file(optionsHdl, fileHdl);
		
		#ifdef MOREVERBOSE
			fprintf(stream, "Modified polyhedral model:\n");
			fflush(stream);
			pet_scop_dump(polyhedralModelPtr[i]);
		#endif
		
		// Be clean for the next file
		fclose(fileHdl);
		free(fileName);
	}
	
	return isl_stat_ok;
}

isl_set *** parse_lattices (FILE * stream, isl_ctx * optionsHdl, unsigned * numLatticesPtr, unsigned numBanks, unsigned dim) {
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
	latticeFileName = (char *)malloc (DIMSTRING * sizeof(char));
	
	if (latticeFileName == NULL) {
		error(stream, "Memory allocation problem  with the lattice file name :(");
		return NULL;
	}
	
	if (sprintf(latticeFileName, "%s%i_dim%i_%s%s", latticeRelativePath, numBanks, dim, numLatticesSuffix, latticeExtension) < 0) {
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
//	translatesPtr = malloc ((*numLatticesPtr * numBanks) * sizeof (isl_set *) );
	
	translatesPtr = (isl_set ***)malloc ((*numLatticesPtr) * sizeof (isl_set **) );
	
	if (translatesPtr == NULL) {
		error(stream, "Memory allocation problem :(");
		return NULL;
	}
	
	// Now we read all the translates for all the lattices
	for (int i = 1; i <= *numLatticesPtr; i++) {
		translatesPtr[i-1] = (isl_set **)malloc (numBanks * sizeof(isl_set *));
		
		
		if (translatesPtr[i-1] == NULL) {
			error(stream, "Memory allocation problem :(");
			return NULL;
		}
		
		for (int j = 1; j <= numBanks; j++) {
			latticeFileName = (char *)malloc (DIMSTRING * sizeof(char));
			
			if (latticeFileName == NULL) {
				error(stream, "Memory allocation problem :(");
				return NULL;
			}
			
			if (sprintf(latticeFileName, latticeFormat, 
				latticeRelativePath, numBanks,dim,  i, j, latticeExtension) < 0) {
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


