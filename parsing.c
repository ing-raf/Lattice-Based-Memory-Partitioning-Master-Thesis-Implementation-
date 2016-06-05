/**
 * @file parsing.c
 *
 * @brief Implementation of functions employed to parse the input files
 */

#ifdef MOREVERBOSE
#define VERBOSE
#endif

#include<stdlib.h>
#include<string.h>

#include "config.h"
#include "support.h"
#include "partitioning.h"

#define DIMSTRING 100

#define FIXED_BANK_LATENCY 		"Fixed"
#define VARIABLE_BANL_LATENCY 	"Variable"

const char * architectureRelativePath = "./Architectures/";
const char * architectureExtension = ".txt";
const char * allocationRelativePath = "./Allocations/";
const char * allocationExtension = ".txt";
const char * parametersRelativePath = "./Sources/Parameters/";
const char * parametersExtension = ".txt";
const char * sourceRelativePath = "../Tests/source-demos/";
const char * sourceExtension = ".c";
const char * scheduleRelativePath = "../Tests/polyhedral-extraction/outputs/"; 
const char * scheduleExtension = ".isl.schedule";
const char * latticeRelativePath = "./Lattices/";
const char * numLatticesSuffix = "numLattices";
const char * latticeExtension = ".txt";
const char * latticeFormat = "%s%i_dim%i_lattice%i_translate%i%s";

/**
 * @brief      Subroutine that parses a UMA architecture
 *
 * @param      stream            The stream for the output messages
 * @param      fileHdl           The handle to the file containing the
 *                               description of the architecture
 * @param      numProcessorsPtr  The number of processors in the architecture
 * @param[out] numBanksPtr       The number of memory banks in the architecture
 *
 * @return     isl_stat_ok if no problem occurs, isl_stat_error otherwise
 */
isl_stat parse_architecture_UMA(FILE * stream, FILE * fileHdl, unsigned * numProcessorsPtr, unsigned * numBanksPtr) {
	// Reading the number of processors
	fscanf(fileHdl, "Number of processors: %u ", numProcessorsPtr);
	
	fprintf(stream, "Number of processors in the architecture: %u\n", *numProcessorsPtr);

	// Reading the number of memory banks
	fscanf(fileHdl, "Number of memory banks: %u ", numBanksPtr);
	
	fprintf(stream, "Number of memory banks in the architecture: %u\n", *numBanksPtr);
}

/**
 * @brief      Subroutine that parses the part of the description of the
 *             architecture related to the service time of the memory banks
 *
 *             If the service time of the memory banks is fixed, the array of
 *             service latencies will be filled with the same fixed value for
 *             all the memory banks.
 *
 * @param      stream          The stream for the output messages
 * @param      fileHdl         The handle to the file containing the description
 *                             of the architecture
 * @param      numBanks        The number of memory banks in the architecture
 * @param      bankLatencyPtr  The array of the service latency of each memory
 *                             bank
 *
 * @return     isl_stat_ok if no problem occurs, isl_stat_error otherwise
 */
isl_stat parse_architecture_bank_latency (FILE * stream, FILE * fileHdl,  unsigned numBanks, unsigned ** bankLatencyPtr) {
	char * latencyString = NULL;
	// Array of bank latencies
	unsigned * bankLatency = NULL;

	bankLatency = (unsigned *)malloc(numBanks * sizeof(unsigned));

	if (bankLatency == NULL) {
		error(stream, "Memory allocation problem with the bank latencies array :(");
		return isl_stat_error;
	}

	latencyString = (char *)malloc(DIMSTRING * sizeof(char));

	if (latencyString == NULL) {
		error(stream, "Memory allocation problem with the bank latency type :(");
		return isl_stat_error;
	}

	fscanf(fileHdl, "Bank latency: %s ", latencyString);

	if (strcmp(latencyString, FIXED_BANK_LATENCY) == 0) {

		fscanf(fileHdl, "Bank latency value: %u ", &(bankLatency[0]));

		for (unsigned i = 1; i < numBanks; i++)
			bankLatency[i] = bankLatency[0];

	} else if (strcmp(latencyString, VARIABLE_BANL_LATENCY) == 0) {

		fscanf(fileHdl, "Latency of each memory bank: ");

		for(unsigned i = 0; i < numBanks; i++)
			fscanf(fileHdl, "%u ", &(bankLatency[i]));

	} else {
		error(stream, "Unsupported bank latencies format");
		return isl_stat_error;
	}

	fprintf(stream, "Memory bank #\t\tLatency\n");

	for(unsigned i = 0; i < numBanks; i++)
		fprintf(stream, "%u\t\t\t%u\n", i, bankLatency[i]);

	// Assigning output
	*bankLatencyPtr = bankLatency;

	// Be clean
	free(latencyString);

	return isl_stat_ok;
	}

/**
 * @brief      Subroutine that parses a generic NUMA architecture
 *
 * @param      stream                   The stream for the output messages
 * @param      fileHdl                  The handle to the file containing the
 *                                      description of the architecture
 * @param[out] numProcessorsPtr         The number of processors in the
 *                                      architecture
 * @param[out] numBanksPtr              The number of memory banks in the
 *                                      architecture
 * @param[out] bankLatencyPtr           The array of the service latency of each
 *                                      memory bank
 * @param[out] processorToBankDelayPtr  The array of the delays incurred by a
 *                                      specified processor to access a
 *                                      specified memory bank
 *
 * @return     isl_stat_ok if no problem occurs, isl_stat_error otherwise
 */
isl_stat parse_architecture_GNUMA(FILE * stream, FILE * fileHdl, unsigned * numProcessorsPtr, unsigned * numBanksPtr, unsigned ** bankLatencyPtr, unsigned *** processorToBankDelayPtr) {
	// Array of access latencies from a processor to a bank
	unsigned ** processorToBankDelay = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_error; 

	// Reading the number of processors
	fscanf(fileHdl, "Number of processors: %u ", numProcessorsPtr);
	
	fprintf(stream, "Number of processors in the architecture: %u\n", *numProcessorsPtr);

	// Reading the number of memory banks
	fscanf(fileHdl, "Number of memory banks: %u ", numBanksPtr);
	
	fprintf(stream, "Number of memory banks in the architecture: %u\n", *numBanksPtr);

	// Reading the memrory bank latencies
	outcome = parse_architecture_bank_latency(stream, fileHdl, *numBanksPtr, bankLatencyPtr);

	if (outcome == isl_stat_error)
		return isl_stat_error;

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

	fprintf(stream, "Processor #\tMemory banks #\t\tLatency\n");

	for(unsigned i = 0; i < *numProcessorsPtr; i++) 
		for(unsigned j = 0; j < *numBanksPtr; j++)
			fprintf(stream, "%u\t\t%u\t\t\t%u\n", i, j, processorToBankDelay[i][j]);

	// Assigning output
	*processorToBankDelayPtr = processorToBankDelay;
}

/**
 * @brief      Function that reads and parses the file containing the specified
 *             architecture
 *
 * @param      stream                   The stream for the output messages
 * @param      name                     The name of the file containing the
 *                                      desctiption of the architecture
 * @param      archType                 The type of architecture
 * @param[out] numProcessorsPtr         The number of processors in the
 *                                      architecture
 * @param[out] numBanksPtr              The number of memory banks in the
 *                                      architecture
 * @param[out] bankLatencyPtr           The array of the service latency of each
 *                                      memory bank
 * @param[out] processorToBankDelayPtr  The array of the delays incurred by a
 *                                      specified processor to access a
 *                                      specified memory bank
 *
 * @return     isl_stat_ok if no problem occurs, isl_stat_error otherwise
 */
isl_stat parse_architecture (FILE * stream, char * name, architecture_type * archType, unsigned * numProcessorsPtr, unsigned * numBanksPtr, unsigned ** bankLatencyPtr, unsigned *** processorToBankDelayPtr) {
	// Name of the file containing the description of the architecture with relative path
	char * fileName = NULL;
	// Handle to the file containing the description of the architecture
	FILE * fileHdl = NULL;
	// String containing the type of architecture read from the architecture file
	char * archTypeString = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_error;

	fileName = (char *)malloc(DIMSTRING * sizeof(char));

	if (fileName == NULL) {
		error(stream, "Memory allocation problem with the architecture file name :(");
		return isl_stat_error;
	}

	fileName[0] = '\0';

	if (sprintf(fileName, "%s%s%s", architectureRelativePath, name, architectureExtension) < 0) {
		error(stream, "Problem when building the name of the file containing the description of the architecture");
		return isl_stat_error;
	}

	fileHdl = fopen(fileName, "r");

	if (fileHdl == NULL) {
		error(stream, "Architecture file not found");
		return isl_stat_error;
	}
	
	// Reading the architecture type
	archTypeString = (char *)malloc(DIMSTRING * sizeof(char));

	if (archTypeString == NULL) {
		error(stream, "Memory allocation problem with the architecture type :(");
		return isl_stat_error;
	}

	fscanf(fileHdl, "Architecture type: %s ", archTypeString);
	
	fprintf(stream, "Architecture: %s\n", archTypeString);

	if (strcmp(archTypeString, "UMA") == 0) {
		*archType = UMA;

		outcome = parse_architecture_UMA(stream, fileHdl, numProcessorsPtr, numBanksPtr);

		*processorToBankDelayPtr = NULL;
		*bankLatencyPtr = NULL;		
	} else if (strcmp(archTypeString, "GNUMA") == 0) {
		*archType = GNUMA;

		outcome = parse_architecture_GNUMA(stream, fileHdl, numProcessorsPtr, numBanksPtr, bankLatencyPtr, processorToBankDelayPtr);

		if (outcome == isl_stat_error)
			return isl_stat_error;
	} else {
		error(stream, "Unsupported architecture");
		return isl_stat_error;
	}

	// Be clean
	free(archTypeString);
	fclose(fileHdl);
	free(fileName);

	return isl_stat_ok;
}

/**
 * @brief         Function that reads and parses the file containing the
 *                specified allocation of processors to tasks in an UMA
 *                architecture.
 *
 *                The input file contains the number of working processors, to
 *                allow employing allocations with fewer processors than the
 *                architecture, that is to leave unused some processors. The
 *                input file contains also the number of executing tasks, to
 *                check if the number of sources is equal to the number of
 *                executing tasks. The input file contains the array specifying
 *                the number of processors assigned to each task.
 *
 * @param         stream            The stream for the output messages
 * @param         name              The name of the file containing the description
 *                                  of the allocation
 * @param[in,out] numProcessorsPtr  In input, the number of processors available
 *                                  in the architecture. In output, the number
 *                                  of allocated processors
 * @param         numTasks          The number of source file provided, that is
 *                                  checked to be equal to the number of
 *                                  executing tasks
 * @param[out]    nPtr              The array containing the number of processors
 *                                  assigned to each task
 *
 * @return        isl_stat_ok if no problem occurs, isl_stat_error otherwise
 */
isl_stat parse_processors_allocation_UMA (FILE * stream, char * name, unsigned * numProcessorsPtr, unsigned numTasks, unsigned ** nPtr) {
	// Name of the file containing the description of the allocation of the processors with relative path
	char * fileName = NULL;
	// Handle to the file containing the description of the allocation
	FILE * fileHdl = NULL;
	// Number of working processors
	unsigned numWorkingProcessors = 0;
	// Number of executing tasks
	unsigned numExecutingTasks = 0;
	// Array containing the number of processors allocated to each task
	unsigned * n = NULL;

	fileName = (char *)malloc(DIMSTRING * sizeof(char));

	if (fileName == NULL) {
		error(stream, "Memory allocation problem with the allocation file name :(");
		return isl_stat_error;
	}

	fileName[0] = '\0';

	if (sprintf(fileName, "%s%s%s", allocationRelativePath, name, allocationExtension) < 0)	{
		error(stream, "Problem when building the name of the file containing the description of the allocation");
		return isl_stat_error;
	}

	fileHdl = fopen(fileName, "r");

	if (fileHdl == NULL) {
		error(stream, "Allocation file not found");
		return isl_stat_error;
	}
	
	// Reading the number of working processors
	fscanf(fileHdl, "Number of working processors: %u ", &numWorkingProcessors);
	
	if (*numProcessorsPtr < numWorkingProcessors) {
		error(stream, "There are not enough processors for this allocation");
		return isl_stat_error;
	}

	*numProcessorsPtr = numWorkingProcessors;

	fprintf(stream, "Number of working processors: %u\n", *numProcessorsPtr);

	// Reading the number of tasks
	fscanf(fileHdl, "Number of executing tasks: %u ", &numExecutingTasks);
	
	if (numExecutingTasks != numTasks) {
		error(stream, "The number of source files provided is not equal to the number of tasks");
		return isl_stat_error;
	}

	fprintf(stream, "Number of executing tasks: %u\n", numTasks);

	// Reading the number of processors assigned to each task
	n = (unsigned *)malloc(numTasks * sizeof(unsigned));

	if (n == NULL) {
		error(stream, "Memory allocation problem with the number of processors array :(");
		return isl_stat_error;
	}	

	fscanf(fileHdl, "Processors assigned to each task: ");

	for(unsigned i = 0; i < numTasks; i++)
		fscanf(fileHdl, "%u ", &(n[i]));

	fprintf(stream, "Task #\tProcessors #\n");

	for(unsigned i = 0; i < numTasks; i++) 
			fprintf(stream, "%u\t%u\n", i, n[i]);

	// Be clean
	fclose(fileHdl);
	free(fileName);

	// Assigning output
	*nPtr = n;

	return isl_stat_ok;
}

/**
 * @brief         Function that reads and parses the file containing the
 *                specified allocation of processors to tasks in a NUMA
 *                architecture.
 *
 *                The input file contains the number of working processors, to
 *                allow employing allocations with fewer processors than the
 *                architecture, that is to leave unused some processors. The
 *                input file contains also the number of executing tasks, to
 *                check if the number of sources is equal to the number of
 *                executing tasks. The input file contains the array specifying
 *                the task ID of the task executing on each processor. Other
 *                arrays are computed fron this array.
 *
 * @param         stream              The stream for the output messages
 * @param         name                The name of the file containing the
 *                                    description of the allocation
 * @param[in,out] numProcessorsPtr    In input, the number of processors
 *                                    available in the architecture. In output,
 *                                    the number of allocated processors
 * @param         numTasks            The number of source file provided, that
 *                                    is checked to be equal to the number of
 *                                    executing tasks
 * @param[out]    taskOnProcessorPtr  The array containing the task ID of the
 *                                    task executing on each processor
 * @param[out]    taskOffsetPtr       The array containing the processor offset
 *                                    for each task
 * @param[out]    nPtr                The aarray containing the number of
 *                                    processors assigned to each task
 *
 * @return        isl_stat_ok if no problem occurs, isl_stat_error otherwise
 */
isl_stat parse_processors_allocation_NUMA (FILE * stream, char * name, unsigned * numProcessorsPtr, unsigned numTasks, unsigned ** taskOnProcessorPtr, unsigned ** taskOffsetPtr, unsigned ** nPtr) {
	// Name of the file containing the description of the allocation of the processors with relative path
	char * fileName = NULL;
	// Handle to the file containing the description of the allocation
	FILE * fileHdl = NULL;
	// Number of working processors
	unsigned numWorkingProcessors = 0;
	// Number of executing tasks
	unsigned numExecutingTasks = 0;
	// Array containing the ID of the task executing of the processor specified as index
	unsigned * taskOnProcessor = NULL;
	// Array of processor offsets for each task
	unsigned * taskOffset = NULL;
	// Array containing the number of processors allocated to each task
	unsigned * n = NULL;
	// Task whose processor offset and number of procesors is being computed
	unsigned currentTask = 0;

	fileName = (char *)malloc(DIMSTRING * sizeof(char));

	if (fileName == NULL) {
		error(stream, "Memory allocation problem with the allocation file name :(");
		return isl_stat_error;
	}

	fileName[0] = '\0';

	if (sprintf(fileName, "%s%s%s", allocationRelativePath, name, allocationExtension) < 0)	{
		error(stream, "Problem when building the name of the file containing the description of the allocation");
		return isl_stat_error;
	}

	fileHdl = fopen(fileName, "r");

	if (fileHdl == NULL) {
		error(stream, "Allocation file not found");
		return isl_stat_error;
	}
	
	// Reading the number of working processors
	fscanf(fileHdl, "Number of working processors: %u ", &numWorkingProcessors);
	
	if (*numProcessorsPtr < numWorkingProcessors) {
		error(stream, "There are not enough processors for this allocation");
		return isl_stat_error;
	}

	*numProcessorsPtr = numWorkingProcessors;

	fprintf(stream, "Number of working processors: %u\n", *numProcessorsPtr);

	// Reading the number of tasks
	fscanf(fileHdl, "Number of executing tasks: %u ", &numExecutingTasks);
	
	if (numExecutingTasks != numTasks) {
		error(stream, "The number of source files provided is not equal to the number of tasks");
		return isl_stat_error;
	}

	fprintf(stream, "Number of executing tasks: %u\n", numTasks);

	// Reading the task executing on each processor
	taskOnProcessor = (unsigned *)malloc(numWorkingProcessors * sizeof(unsigned));

	if (taskOnProcessor == NULL) {
		error(stream, "Memory allocation problem with the processors allocation array :(");
		return isl_stat_error;
	}

	fscanf(fileHdl, "Task ID executing on each processor: ");

	for(unsigned i = 0; i < numWorkingProcessors; i++)
		fscanf(fileHdl, "%u ", &(taskOnProcessor[i]));

	fprintf(stream, "Processor #\tTask\n");

	for(unsigned i = 0; i < numWorkingProcessors; i++)
		fprintf(stream, "%u\t\t%u\n", i, taskOnProcessor[i]);

	// Be clean
	fclose(fileHdl);
	free(fileName);
	
	taskOffset = (unsigned *)malloc(numTasks * sizeof(unsigned));

	if (taskOnProcessor == NULL) {
		error(stream, "Memory allocation problem with the task offset array :(");
		return isl_stat_error;
	}

	n = (unsigned *)malloc(numTasks * sizeof(unsigned));

	if (n == NULL) {
		error(stream, "Memory allocation problem with the number of processors array :(");
		return isl_stat_error;
	}	

	currentTask = taskOnProcessor[0];
	n[currentTask] = 1;
	taskOffset[currentTask] = 0;

	// Computing the other allocation arrays
	for (unsigned i = 1; i < numWorkingProcessors; i++) {
		
		if (taskOnProcessor[i] == currentTask) {
			n[currentTask] = n[currentTask] + 1;
		}
		else {
			currentTask = taskOnProcessor[i];
			n[currentTask] = 1;
			taskOffset[currentTask] = i ;
		}

	}

	fprintf(stream, "Task #\tProcessors #\tOffset\n");

	for(unsigned i = 0; i < numTasks; i++) 
			fprintf(stream, "%u\t%u\t\t%u\n", i, n[i], taskOffset[i]);

	// Assigning output
	*taskOnProcessorPtr = taskOnProcessor;
	*taskOffsetPtr = taskOffset;
	*nPtr = n;

	return isl_stat_ok;
}

/**
 * @brief      Function that reads and parses an input file containing the
 *             number and all the values of the parameters, for each input
 *             source file
 *
 * @param      taskNames           The array of the task names
 * @param      paramFileNames      The array of the parameter file names
 * @param      numTasks            The number of executing tasks
 * @param[out] parametersArrayPtr  The array containing the array of parameters for
 *                                 each task
 *
 * @return     isl_stat_ok if no problem occurs, isl_stat_error otherwise
 */
isl_stat parse_parameters (char ** taskNames, char ** paramFileNames, unsigned numTasks, parameters *** parametersArrayPtr) {
	// Array of the parameters
	parameters ** parametersArray = NULL;
	// Name of the file containing the parameters for the current task
	char * fileName = NULL;
	// Handle to the file containing the description of the allocation
	FILE * fileHdl = NULL;
	// Number of parameters for the current task
	unsigned numParameters = 0;
	// Array containing the parameters value for the current task
	int * values = NULL;

	parametersArray = parameters_array_alloc(numTasks);

	if (parametersArray == NULL)
		return isl_stat_error;

	for (unsigned i = 0; i < numTasks; i++) {

		fileName = (char *)malloc(DIMSTRING * sizeof(char));

		if (fileName == NULL) 
			return isl_stat_error;

		fileName[0] = '\0';

		if (sprintf(fileName, "%s%s%s%s%s", parametersRelativePath, taskNames[i], "/", paramFileNames[i], parametersExtension) < 0)
			return isl_stat_error;

		fileHdl = fopen(fileName, "r");

		if (fileHdl == NULL) 
			return isl_stat_error;

		// Reading the number of parameters
		fscanf(fileHdl, "Number of parameters:  %u ", &numParameters);

		// Reading the parameters value
		fscanf(fileHdl, "Parameters values: ");

		values = (int *)malloc(numParameters * sizeof(int));

		if (values == NULL)
			return isl_stat_error;

		for (unsigned j = 0; j < numParameters; j++)
			fscanf(fileHdl, "%i ", &(values[j]));

		if (parameters_array_insert(parametersArray, i, numParameters, values) == isl_stat_error)
			return isl_stat_error;

		// Be clean
		fclose(fileHdl);
		free(fileName);
	}

	// Assigning output
	*parametersArrayPtr = parametersArray;

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
		
		#ifdef VERBOSE
			fprintf(stream, "Reading file %s\n", fileName);
		#endif

		fileHdl = fopen(fileName, "r");
		
		if (fileHdl == NULL) {
			error(stream, "Schedule file not found");
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
		error(stream, "Lattices number file not found");
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
				error(stream, "Lattice file not found");
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


