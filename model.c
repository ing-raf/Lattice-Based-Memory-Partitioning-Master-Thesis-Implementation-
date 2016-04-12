/*
 * Implementation of function employed to manipulate the type representing the
 * modified poyhedral model
 */ 
#include<stdlib.h>

#include "model.h"

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
	}
	
	return array;
	
}

void manipulated_polyhedral_model_array_free(manipulated_polyhedral_model ** array, unsigned numTasks) {
	for (int i = 0; i < numTasks; i++)
		free(array[i]);
	
	free(array);
}
