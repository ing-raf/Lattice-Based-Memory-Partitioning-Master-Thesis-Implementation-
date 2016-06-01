#ifdef MOREVERBOSE
	#define VERBOSE
#endif

#include<glpk.h>

#include "model.h"
#include "config.h"
#include "support.h"
#include "partitioning.h"

#define DIMSTRING 100

#define GLP_OK 0
#define GLP_NO_READ_DATA 1

const char * MLPRelativePath = "./MLP/";
const char * modelExtension =  ".mod";
const char * dataExtension =  ".dat";

int redirectStream (void * stream, const char * string) {
	FILE * output = (void *)stream;
	fputs(string, output);
	return 1;
}

isl_stat inputMILP (unsigned numBanks, dataset_type_array * datasetTypesPtr, unsigned numLattice, double currentBest, unsigned bankLatency, unsigned ** delta) {
	// File name with relative path
	char * fileName = NULL;
	// Handle to the file to be written
	FILE * dataFileHdl = NULL;

	fileName = malloc(DIMSTRING * sizeof(char));

	if (fileName == NULL)
		return isl_stat_error;


	if (sprintf(fileName, "%s%s%u%s", MLPRelativePath, "lattice", numLattice, dataExtension) < 0)
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
	fprintf(dataFileHdl, "param minLatency := %f;\n", currentBest);
	
	if (numLattice > 0)
		fprintf(dataFileHdl, "param nonFirstLattice := %u;\n", 1);
	else
		fprintf(dataFileHdl, "param nonFirstLattice := %u;\n", 0);

	fprintf(dataFileHdl, "param l := %u;\n", bankLatency);

	fprintf(dataFileHdl, "\n");

	fprintf(dataFileHdl, "param delta := \n");

	for (unsigned p = 0; p < datasetTypesPtr -> numProcessors; p++) 
		for (unsigned b = 0; b < numBanks; b++)
			fprintf(dataFileHdl, "\tp%u\tb%u\t%u\n", p, b, delta[p][b]);

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

/**
 * Naive programming of the MILP problem solving. Builds the GMPL data file for each fundamental lattice, hence reads the GMPL formulation of the model and the data file just written to solve it.
 * @param  stream          Stream for the output messages
 * @param  numBanks        Number of memory banks in the architecture
 * @param  datasetTypesPtr Array containing, for each fundamental lattice, the array of dataset types
 * @param  numLattices     Number of different fundamental lattices
 * @param  bankLatency     Latency of the single memory bank (common for all)
 * @param  bestLattice     (output) Index of the best fundamental lattice
 * @return                 isl_stat_ok if no problem occurs, isl_stat_error otherwise
 */
isl_stat MILPsolve(FILE * stream, unsigned numBanks, dataset_type_array ** datasetTypesPtr, unsigned numLattices, unsigned bankLatency, unsigned * bestLattice, unsigned ** delta) { 
	// Current best value of the cost function
	double currentBest = 0;
	// Result of a GLPK routine
	int glp_outcome = -1;
	// File name with relative path
	char * fileName = NULL;
	// Pointer to the workspace of the translator
	glp_tran * ws = NULL;
	// Pointer to the MILP problem model
	glp_prob * milp = NULL;
	// Pointer to the control parameters structure of the LP solver
	glp_smcp * simplexParams = NULL;
	// Pointer to the control parameters structure of the MILP solver
	glp_iocp * intParams = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_error;

	for (unsigned i = 0; i < numLattices; i++) {
		#ifdef VERBOSE
			info(stream, "Fundamental lattice %u)", i);
		#endif

		// Building the data file
		outcome = inputMILP(numBanks, datasetTypesPtr[i], i, currentBest, bankLatency, delta);

		if (outcome == isl_stat_error)
			return isl_stat_error;

		#ifdef MOREVERBOSE
			glp_term_hook(redirectStream, (void *)stream); // starts the output redirection
		#else
			glp_term_out(GLP_OFF);
		#endif

		// Allocating the problem object
		milp = glp_create_prob();

		if (milp == NULL)
			return isl_stat_error;

		// Allocating the workspace for the translator
		ws = glp_mpl_alloc_wksp();

		if (ws == NULL)
			return isl_stat_error;

		// Reading the model section from the model file
		fileName = malloc(DIMSTRING * sizeof(char));

		if (fileName == NULL)
			return isl_stat_error;

		if (sprintf(fileName, "%s%s%s", MLPRelativePath, "model", modelExtension) < 0)
			return isl_stat_error;

		glp_outcome = glp_mpl_read_model(ws, fileName, GLP_NO_READ_DATA);

		if (glp_outcome != GLP_OK)
			return isl_stat_error;

		free(fileName);

		// Reading the data section from the data file just written
		fileName = malloc(DIMSTRING * sizeof(char));

		if (fileName == NULL)
			return isl_stat_error;

		if (sprintf(fileName, "%s%s%u%s", MLPRelativePath, "lattice", i, dataExtension) < 0)
			return isl_stat_error;

		glp_outcome = glp_mpl_read_data(ws, fileName);

		if (glp_outcome != GLP_OK)
			return isl_stat_error;
		
		free(fileName);

		glp_outcome = glp_mpl_generate(ws, NULL);

		if (glp_outcome != GLP_OK)
			return isl_stat_error;

		// Building the problem object from the translator
		glp_mpl_build_prob(ws, milp);
		
		// Solving the model
		simplexParams = (glp_smcp *) malloc(sizeof(glp_smcp));

		if (simplexParams == NULL)
			return isl_stat_error;

		glp_init_smcp(simplexParams);

		// Disable terminal output
		#ifndef MOREVERBOSE
			simplexParams -> msg_lev = GLP_MSG_OFF;
		#endif

 		// LP relaxation
		glp_outcome = glp_simplex(milp, simplexParams);

		if (glp_outcome != GLP_OK)
			return isl_stat_error;

		if (glp_get_status(milp) == GLP_OPT) {
			intParams = (glp_iocp *) malloc(sizeof(glp_iocp));

			if (intParams == NULL)
				return isl_stat_error;

			glp_init_iocp(intParams);

			// Disable terminal output
			#ifndef MOREVERBOSE
				intParams -> msg_lev = GLP_MSG_OFF;
			#endif

			// Branch - and - cut
			glp_intopt(milp, intParams);

			// Retrieving the optimal solution	
			if (glp_mip_status(milp) == GLP_OPT) {
				currentBest = glp_mip_obj_val(milp) - 1;
				*bestLattice = i;

				#ifdef VERBOSE
					news(stream, "New current best lattice");
					} else {

						if (glp_mip_status(milp) == GLP_NOFEAS)
							fprintf(stream, "No integer feasible solution\n");
						else if (glp_mip_status(milp) == GLP_FEAS)
							error(stream, "Integer feasible solution, but too much time to prove optimality\n");
						else if (glp_mip_status(milp) == GLP_UNDEF)
							warning(stream, "Undefined solution");

				#endif
			} // closing bracket of the internal if statement (related to the branch - and - cut) when VERBOSE is undefined
			#ifdef VERBOSE
				} else {

					if (glp_get_status(milp) == GLP_FEAS)
						warning(stream, "The solution of the LP relaxation is feasible");
					else if (glp_get_status(milp) == GLP_INFEAS)
						warning(stream, "The solution of the LP relaxation is infeasible");
					else if (glp_get_status(milp) == GLP_NOFEAS)
						warning(stream, "The LP relaxation has no feasible solution");
					else if (glp_get_status(milp) == GLP_UNBND)
						warning(stream, "The LP relaxation has unbounded solution");
					else if (glp_get_status(milp) == GLP_UNDEF)
						warning(stream, "The LP relaxation has no defined solution");

			#endif
		} // closing bracket of the external if statement (related to the LP relaxation) when VERBOSE is undefined

		#ifdef MOREVERBOSE
			glp_term_hook(NULL, NULL); // stops the output redirection
		#endif
		// Be clean
		free(intParams);
		free(simplexParams);
		glp_mpl_free_wksp(ws);
		glp_delete_prob(milp);
	}

	return isl_stat_ok;
}