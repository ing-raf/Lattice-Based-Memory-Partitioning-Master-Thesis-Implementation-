/*
 * Implementation of function employed to manipulate the type representing the
 * modified poyhedral model
 */ 
#include<stdlib.h>

#include "model.h"
#include "config.h"

parameters ** parameters_array_alloc(unsigned numTasks) {
	// Array to be allocated
	parameters ** parametersArray = NULL;

	parametersArray = (parameters **)malloc(numTasks * sizeof(parameters *));

	if (parametersArray == NULL)
		return NULL;

	for (unsigned i = 0; i < numTasks; i++) {
		parametersArray[i] = NULL;
	}

	return parametersArray;
}

isl_stat parameters_array_insert(parameters ** parametersArray, unsigned taskNum, unsigned numParameters, unsigned * values) {
	parametersArray[taskNum] = (parameters *) malloc(sizeof(parameters));

	if (parametersArray[taskNum] == NULL)
		return isl_stat_error;

	parametersArray[taskNum] -> numParameters = numParameters;
	parametersArray[taskNum] -> values = (int *)malloc(numParameters * sizeof(int));

	if (parametersArray[taskNum] -> values == NULL) 
		return isl_stat_error;

	// This gives this function the character of __isl_take
	for (unsigned i = 0; i < numParameters; i++)
		parametersArray[taskNum] -> values[i] = values[i];

	free(values);

	return isl_stat_ok;
}

void parameters_array_free(parameters ** parametersArray, unsigned numTasks) {

	for (unsigned i = 0; i < numTasks; i++) {
		free(parametersArray[i] -> values);
		free(parametersArray[i]);
	}

	free(parametersArray);

}

manipulated_polyhedral_model ** manipulated_polyhedral_model_array_alloc(unsigned numTasks) {
	// Array to be allocated
	manipulated_polyhedral_model ** array = NULL;
	
	array = malloc(numTasks * sizeof(manipulated_polyhedral_model *));
	
	if (array == NULL)
		return array;
	
	for (int i = 0; i < numTasks; i++) {
		array[i] = malloc(sizeof(manipulated_polyhedral_model));
		
		// If one fails, free all
		if (array[i] == NULL) {
			
			for (int j = 0; j < i; j++)
				free(array[j]);
			
			free(array);
			
			return NULL;
		}

		array[i] -> parallelIteratorPos = 0;
		array[i] -> instanceSet = NULL;
		array[i] -> flattenedSchedule = NULL;
		array[i] -> allocation = NULL;
		array[i] -> remappedMayReads = NULL;
		array[i] -> remappedMayWrites = NULL;
		array[i] -> remappedMustWrites = NULL;
		array[i] -> linearizedSchedule = NULL;
	}
	
	return array;
	
}

void manipulated_polyhedral_model_array_free(manipulated_polyhedral_model ** array, unsigned numTasks) {
	for (int i = 0; i < numTasks; i++)
		free(array[i]);
	
	free(array);
}

dataset_type_array * dataset_type_array_alloc(unsigned numTranslates, unsigned numProcessors) {
	// Pointer to the newly allocated array
	dataset_type_array * datasetArray = NULL;
	
	datasetArray = (dataset_type_array *)malloc(sizeof(dataset_type_array));
	
	if (datasetArray == NULL)
		return NULL;
	
	datasetArray -> datasetArray = NULL;
	datasetArray -> numTypes = 0;
	datasetArray -> numTranslates = numTranslates;
	datasetArray -> numProcessors = numProcessors;
	
	return datasetArray;
}

isl_bool matrix_compare (unsigned, unsigned, unsigned **, unsigned **);
void matrix_fprintf(FILE *, unsigned, unsigned, unsigned **);

isl_stat dataset_type_array_add(dataset_type_array * datasetArray, unsigned ** newDatasetTypeInstance) {
	// Boolean variable for the linear search
	isl_bool found = isl_bool_false;
	
	if (datasetArray -> numTypes == 0) { // dataset type surely not present
		datasetArray -> numTypes = 1;
		datasetArray -> datasetArray = (dataset_type **)malloc(sizeof(dataset_type *));
		
		if (datasetArray -> datasetArray == NULL)
			return isl_stat_error;

		datasetArray -> datasetArray[0] = (dataset_type *)malloc(sizeof(dataset_type));

		if (datasetArray -> datasetArray[0] == NULL)
			return isl_stat_error;
		
		datasetArray -> datasetArray[0] -> access = newDatasetTypeInstance;
		datasetArray -> datasetArray[0] -> n = 1;
		
		return isl_stat_ok;
	}
	
	for (int i = 0; i < datasetArray -> numTypes && found == isl_bool_false; i++) {
		
		if (matrix_compare(datasetArray -> numTranslates, datasetArray -> numProcessors, 
			datasetArray -> datasetArray[i] -> access, newDatasetTypeInstance) == isl_bool_true) {
			datasetArray -> datasetArray[i] -> n = datasetArray -> datasetArray[i] -> n + 1;

			found = isl_bool_true;
		}
	}
	
	if (found == isl_bool_false) { // add the new type
		datasetArray -> numTypes = datasetArray -> numTypes + 1;
		datasetArray -> datasetArray = (dataset_type **)realloc(datasetArray -> datasetArray, (datasetArray -> numTypes) * sizeof(dataset_type *));
		
		if (datasetArray -> datasetArray == NULL)
			return isl_stat_error;

		datasetArray -> datasetArray[datasetArray -> numTypes - 1] = (dataset_type *)malloc(sizeof(dataset_type));

		if (datasetArray -> datasetArray[datasetArray -> numTypes - 1] == NULL)
			return isl_stat_error;
		
		datasetArray -> datasetArray[datasetArray -> numTypes - 1] -> access = newDatasetTypeInstance;
		datasetArray -> datasetArray[datasetArray -> numTypes - 1] -> n = 1;

		return isl_stat_ok;
		
	} 

	return isl_stat_ok;
	
}

void dataset_type_array_fprintf(FILE * stream, dataset_type_array * datasetArray) {
	
	for (unsigned i = 0; i < datasetArray -> numTypes; i++) {
		fprintf(stream, "Dataset type %u: \n", i + 1);
		
		matrix_fprintf(stream, datasetArray -> numTranslates, datasetArray -> numProcessors, datasetArray -> datasetArray[i] -> access);

		fprintf(stream, "Repetitions of the type: %u\n", datasetArray -> datasetArray[i] -> n);
	}
}

void dataset_type_array_free(dataset_type_array * datasetArray) {
	
	for (unsigned i = 0; i < datasetArray -> numTypes; i++)
		free(datasetArray -> datasetArray[i]);
		
	free(datasetArray);
}

isl_bool matrix_compare (unsigned n, unsigned m, unsigned ** A, unsigned ** B) {

	for (unsigned i = 0; i < n; i++)
		for (unsigned j = 0; j < m; j++)
			if (A[i][j] != B[i][j])
				return isl_bool_false;
	
	return isl_bool_true;
}

void matrix_fprintf(FILE * stream, unsigned n, unsigned m, unsigned ** A) {
	
	for (unsigned i = 0; i < n; i++) {
		for (unsigned j = 0; j < m; j++)
			fprintf(stream, "%u ", A[i][j]);
			
			fprintf(stream, "\n");
		}
}