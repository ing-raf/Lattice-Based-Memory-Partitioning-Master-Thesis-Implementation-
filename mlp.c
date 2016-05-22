#include "model.h"
#include "config.h"
#include "partitioning.h"

#define DIMSTRING 100

const char * MLPRelativePath = "./MLP/";
const char * dataExtension =  ".dat";

isl_stat inputMLP (unsigned numBanks, dataset_type_array * datasetTypesPtr, unsigned numLattice, unsigned currentBest, unsigned bankLatency) {
	// File name with relative path
	char * fileName = NULL;
	// Handle to the file to be written
	FILE * dataFileHdl = NULL;

	fileName = malloc(DIMSTRING * sizeof(char));

	if (fileName == NULL)
		return isl_stat_error;


	if (sprintf(fileName, "%s%s%u%s", MLPRelativePath, "lattice", numLattice, dataExtension) < 0)
//		perror(muoio);
		return isl_stat_error;

	dataFileHdl = fopen(fileName, "w");

	if (dataFileHdl == NULL)
		return isl_stat_error;

	// Set of processors
	fprintf(dataFileHdl, "set P :=");

	for (unsigned i = 0; i < datasetTypesPtr -> numProcessors; i++)
		fprintf(dataFileHdl, " p%u", i);

	fprintf(dataFileHdl, ";\n");


	// Set of banks
	fprintf(dataFileHdl, "set B :=");

	for (unsigned i = 0; i < numBanks; i++)
		fprintf(dataFileHdl, " b%u", i);

	fprintf(dataFileHdl, ";\n");
		
	// Set of translates
	fprintf(dataFileHdl, "set T :=");

	for (unsigned i = 0; i < datasetTypesPtr -> numTranslates; i++)
		fprintf(dataFileHdl, " t%u", i);

	fprintf(dataFileHdl, ";\n");

	// Set of dataset types
	fprintf(dataFileHdl, "set D :=");

	for (unsigned i = 0; i < datasetTypesPtr -> numTypes; i++)
		fprintf(dataFileHdl, " d%u", i);

	fprintf(dataFileHdl, ";\n");

	fprintf(dataFileHdl, "\n");

	// Parameters
	fprintf(dataFileHdl, "param minLatency := %u;\n", currentBest);
	
	if (numLattice > 0)
		fprintf(dataFileHdl, "param nonFirstLattice := %u;\n", 1);
	else
		fprintf(dataFileHdl, "param nonFirstLattice := %u;\n", 0);

	fprintf(dataFileHdl, "param l := %u;\n", bankLatency);

	fprintf(dataFileHdl, "\n");

	fprintf(dataFileHdl, "param delta := \n");

	for (unsigned p = 0; p < datasetTypesPtr -> numProcessors; p++) 
		for (unsigned b = 0; b < numBanks; b++)
			fprintf(dataFileHdl, "\tp%u\tb%u\t%u\n", p, b, DELTAS[p][b]);

	fprintf(dataFileHdl, ";\n\n");	

	fprintf(dataFileHdl, "param n := \n");

	for (unsigned i = 0; i < datasetTypesPtr -> numTypes; i++)
		fprintf(dataFileHdl, "\td%u\t%u\n", i, datasetTypesPtr -> datasetArray[i] -> n);

	fprintf(dataFileHdl, ";\n\n");


	fprintf(dataFileHdl, "param mc default 0 := \n");

	for (unsigned d = 0; d < datasetTypesPtr -> numTypes; d++)
		for (unsigned p = 0; p < datasetTypesPtr -> numProcessors; p++)
			for (unsigned t = 0; t < datasetTypesPtr -> numTranslates; t++)
				if (datasetTypesPtr -> datasetArray[d] -> access[p][t] != 0)
					fprintf(dataFileHdl, "\td%u\tp%u\tt%u\t%u\n", d, p, t,  datasetTypesPtr -> datasetArray[d] -> access[p][t]);

	fprintf(dataFileHdl, ";\n\nend;\n");

	fclose(dataFileHdl);
	free(fileName);
}

isl_stat MLPsolve(unsigned numBanks, dataset_type_array ** datasetTypesPtr, unsigned numLattices, unsigned bankLatency) { 
	// Result of a subroutine
	isl_stat outcome = isl_stat_error;

	for (unsigned i = 0; i < numLattices; i++) {
		outcome = inputMLP(numBanks, datasetTypesPtr[i], i, 0, bankLatency);

		if (outcome == isl_stat_error)
			return isl_stat_error;
	}

	return isl_stat_ok;
}