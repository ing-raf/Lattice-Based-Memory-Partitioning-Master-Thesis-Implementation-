#ifdef BARVINOK_ON
	#ifndef MOREVERBOSE
		#define MOREVERBOSE
	#endif
#endif

#ifdef MOREVERBOSE
	#define VERBOSE
#endif

#include <stdlib.h>

#include <isl/union_set.h>
#ifdef BARVINOK_ON
	#include <barvinok/isl.h>
#endif

#include "config.h"
#include "support.h"
#include "partitioning.h"

typedef struct {
	isl_union_set * appliedSchedule;
	isl_union_map * partialLinearization;
	#ifdef MOREVERBOSE
		isl_printer * printer;
	#endif
} linearize_date_params;

typedef struct {
	unsigned count;
} set_cardinality_params;

isl_stat linearize_date(isl_point *, void *);
isl_stat set_cardinality(isl_point *, void *);

isl_stat linearize_dates(FILE * stream, manipulated_polyhedral_model ** modifiedPolyhedralModelPtr, unsigned numTasks) {
	#ifdef VERBOSE
		// Pointer to the printer
		isl_printer * printer = NULL;
	#endif
	// Parameters for the callback function
	linearize_date_params * linearizationParams = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	
	for(int i = 0; i < numTasks; i++) {
		
		#ifdef VERBOSE
			info(stream, "Task %d)", i);
			printer = isl_printer_to_file(isl_union_set_get_ctx(modifiedPolyhedralModelPtr[i] -> instanceSet), stream);
			
			if(printer == NULL) {
				error(stream, "Memory allocation problem :(");
				return isl_stat_error;
			} 
			
			isl_printer_set_indent(printer, moreIndent);
		#endif
		
		linearizationParams = malloc(sizeof(linearize_date_params));
		
		if (linearizationParams == NULL)
			return isl_stat_error;
		
		linearizationParams -> partialLinearization = NULL;
		linearizationParams -> appliedSchedule = isl_union_set_apply(isl_union_set_copy(modifiedPolyhedralModelPtr[i] -> instanceSet), isl_union_map_copy(modifiedPolyhedralModelPtr[i] -> flattenedSchedule));
		
		if (linearizationParams -> appliedSchedule == NULL)
			return isl_stat_error;
		
		#ifdef MOREVERBOSE
			linearizationParams -> printer = printer;
		#endif
		
		#ifdef VERBOSE
			fprintf(stream, "Applied schedule:\n");
			fflush(stream);
			printer = isl_printer_print_union_set(printer, linearizationParams -> appliedSchedule);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			}
			
			fprintf(stream, "\n");
		#endif
		
		outcome = isl_union_set_foreach_point(linearizationParams -> appliedSchedule, linearize_date, (void *)linearizationParams);
		
		if (outcome == isl_stat_error)
			return isl_stat_error;
		
		modifiedPolyhedralModelPtr[i] -> linearizedSchedule = isl_union_map_coalesce(linearizationParams -> partialLinearization);
		
		#ifdef MOREVERBOSE
			fprintf(stream, "Linearized schedule:\n");
			printer = isl_printer_print_union_map(printer, modifiedPolyhedralModelPtr[i] -> linearizedSchedule);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\n");
			fflush(stream);
			isl_printer_free(printer);
		#endif	
		// Be clean
		free(linearizationParams);
	}
	
	return isl_stat_ok;
	
}

isl_stat linearize_date (isl_point * vector, void * user) {
	#ifdef MOREVERBOSE
		// Handle to the output stream
		FILE * stream = NULL;
	#endif
	// Pointer to the input parameters
	linearize_date_params * params = (linearize_date_params *)user;
	// Pointer to the singleton set of the point
	isl_union_set * singletonPtr = NULL;
	// Pointer to the set of lexicographically smaller dates
	isl_union_set * lexLtSetPtr = NULL;
	// Parameters for the callback function
	set_cardinality_params * cardParams = NULL;
	// Pointer to the point representing the linearized date
	isl_point * datePointPtr = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	
	
	#ifdef MOREVERBOSE
		stream = isl_printer_get_file(params -> printer);
		fprintf(stream, "Point being linearized:");
		fflush(stream);
		params -> printer = isl_printer_print_point(params -> printer, vector);
		
		if(params -> printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
	#endif
	
	singletonPtr = isl_union_set_from_point(vector);
	
	if (singletonPtr == NULL)
		return isl_stat_error;
	
	#ifdef MOREVERBOSE
		fprintf(stream, "Singleton set:");
		fflush(stream);
		params -> printer = isl_printer_print_union_set(params -> printer, singletonPtr);
		
		if(params -> printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\nApplied schedule:\n");
		fflush(stream);
		params -> printer = isl_printer_print_union_set(params -> printer, params -> appliedSchedule);
		
		if(params -> printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		}
		
		fprintf(stream, "\n");
	#endif
	
	lexLtSetPtr = isl_union_map_domain(isl_union_set_lex_lt_union_set(isl_union_set_copy(params -> appliedSchedule), isl_union_set_copy(singletonPtr)));
	
	if (lexLtSetPtr == NULL)
		return isl_stat_error;
	
	#ifdef MOREVERBOSE
		fprintf(stream, "Set of lexicographically smaller dates:\n");
		fflush(stream);
		params -> printer = isl_printer_print_union_set(params -> printer, lexLtSetPtr);
		
		if(params -> printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
	#endif
	
	cardParams = malloc(sizeof(set_cardinality_params));
	
	if (cardParams == NULL)
		return isl_stat_error;
	
	cardParams -> count = 0;
	
	outcome = isl_union_set_foreach_point (lexLtSetPtr, set_cardinality, (void *)cardParams);
	
	if (outcome == isl_stat_error)
		return isl_stat_error;
	
	#ifdef BARVINOK_ON // MOREVERBOSE
		fprintf(stream, "Cardinality of the set: %d\n", cardParams -> count);
		fprintf(stream, "Barvinok cardinality:\n");
		fflush(stream);
		isl_pw_qpolynomial_dump(isl_set_card(isl_set_from_union_set(lexLtSetPtr)));
	#endif
	
	datePointPtr = isl_point_zero(isl_space_set_alloc(isl_union_set_get_ctx(singletonPtr), 0, 1));
	datePointPtr = isl_point_set_coordinate_val(datePointPtr, isl_dim_set, 0, isl_val_int_from_ui(isl_union_set_get_ctx(singletonPtr), cardParams -> count));
	
	if (datePointPtr == NULL)
		return isl_stat_error;

	#ifdef MOREVERBOSE
		fprintf(stream, "Linearized date point: ");
		fflush(stream);
		params -> printer = isl_printer_print_point(params -> printer, datePointPtr);
		
		if(params -> printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		}
		
		fprintf(stream, "\n");
	#endif
	
	if (params -> partialLinearization == NULL)
		params -> partialLinearization = isl_union_map_from_domain_and_range(singletonPtr, isl_union_set_from_point(datePointPtr));
	else
		params -> partialLinearization = isl_union_map_union(params -> partialLinearization, isl_union_map_from_domain_and_range(singletonPtr, isl_union_set_from_point(datePointPtr)));
	
	#ifdef MOREVERBOSE
		fprintf(stream, "Partial linearization:\n");
		fflush(stream);
		params -> printer = isl_printer_print_union_map(params -> printer, params -> partialLinearization);
		
		if(params -> printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
	#endif
	
	//Be clean
	free(cardParams);
	isl_union_set_free(lexLtSetPtr);

	return isl_stat_ok;
}

isl_stat set_cardinality (isl_point * vector, void * user) {
	// Pointer to the input parameters
	set_cardinality_params * params = (set_cardinality_params *)user;
	
	params -> count += 1;
	
	return isl_stat_ok;
}

isl_set * concurrent_dataset_build (FILE * stream, manipulated_polyhedral_model ** modifiedPolyhedralModelPtr, isl_union_set ** polyhedralSlicePtr, unsigned numTasks) {
	// Array of the datasets of each task
	isl_set ** datasetPtr = NULL;
	// Pointer to a part of the dataset of the current task
	isl_union_set * partialDatasetPtr = NULL;
	#ifdef MOREVERBOSE
		// Pointer to the printer
		isl_printer * printer = NULL;
	#endif
	isl_set * currentDatasetPtr = NULL; 
	
	datasetPtr = malloc(numTasks * sizeof(isl_set *));
	
	if(datasetPtr == NULL) {
		error(stream, "Memory allocation problem for the dataset array");
		return NULL;
	}
	
	for (int i = 0; i < numTasks; i++) {
		#ifdef MOREVERBOSE
			info(stream, "Task %d)", i);
		#endif
		datasetPtr[i] = isl_set_empty(isl_set_get_space(isl_set_from_union_set(isl_union_map_range(isl_union_map_copy(modifiedPolyhedralModelPtr[i] -> remappedMayReads)))));
		
		if (datasetPtr[i] == NULL) {
			error(stream, "Memory allocation problem for the dataset :(");
			return NULL;
		} 
		
		if (isl_union_map_is_empty(modifiedPolyhedralModelPtr[i] -> remappedMayReads) == isl_bool_false) {
			partialDatasetPtr = isl_union_set_apply(isl_union_set_copy(polyhedralSlicePtr[i]), isl_union_map_copy(modifiedPolyhedralModelPtr[i] -> remappedMayReads));
			
			if (isl_union_set_is_empty(partialDatasetPtr) == isl_bool_false)
				datasetPtr[i] = isl_set_union(datasetPtr[i], isl_set_from_union_set(partialDatasetPtr));
		}
		
		if (isl_union_map_is_empty(modifiedPolyhedralModelPtr[i] -> remappedMayWrites) == isl_bool_false) {
			partialDatasetPtr = isl_union_set_apply(isl_union_set_copy(polyhedralSlicePtr[i]), isl_union_map_copy(modifiedPolyhedralModelPtr[i] -> remappedMayWrites));
			
			if (isl_union_set_is_empty(partialDatasetPtr) == isl_bool_false)
				datasetPtr[i] = isl_set_union(datasetPtr[i], isl_set_from_union_set(partialDatasetPtr));
		}
		
		if (isl_union_map_is_empty(modifiedPolyhedralModelPtr[i] -> remappedMustWrites) == isl_bool_false) {
			partialDatasetPtr = isl_union_set_apply(isl_union_set_copy(polyhedralSlicePtr[i]), isl_union_map_copy(modifiedPolyhedralModelPtr[i] -> remappedMustWrites));
			
			if (isl_union_set_is_empty(partialDatasetPtr) == isl_bool_false)
				datasetPtr[i] = isl_set_union(datasetPtr[i], isl_set_from_union_set(partialDatasetPtr));
		}
		
		if (datasetPtr[i] == NULL) {
			error(stream, "Problem during dataset construction");
			return NULL;
		} 
		
		#ifdef MOREVERBOSE
			printer = isl_printer_to_file(isl_set_get_ctx(datasetPtr[i]), stream);
			
			if(printer == NULL) {
				error(stream, "Memory allocation problem :(");
				return NULL;
			} 
			
			printer = isl_printer_set_indent(printer, moreIndent);
			
			fprintf(stream, "Dataset: ");
			printer = isl_printer_print_set(printer, datasetPtr[i]);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return NULL;
			} 
			
			fprintf(stream, "\n");
			fflush(stream);
		#endif
	}
	
	currentDatasetPtr = datasetPtr[0];
	
	for (int i = 1; i < numTasks; i++)
		currentDatasetPtr = isl_set_union(currentDatasetPtr, datasetPtr[i]);
	
	return isl_set_coalesce(currentDatasetPtr);
}

isl_set ** instant_local_dataset_build (FILE * stream, manipulated_polyhedral_model ** modifiedPolyhedralModelPtr, isl_union_set ** instantLocalSlicePtr, unsigned * taskOnProcessor, unsigned numProcessors) {
	// Array of the datasets of each processor
	isl_set ** datasetPtr = NULL;
	// Pointer to a part of the dataset of the current processor
	isl_union_set * partialDatasetPtr = NULL;
	#ifdef MOREVERBOSE
		// Pointer to the printer
		isl_printer * printer = NULL;
	#endif
	isl_set * currentDatasetPtr = NULL; 
	
	datasetPtr = malloc(numProcessors * sizeof(isl_set *));
	
	if(datasetPtr == NULL) {
		error(stream, "Memory allocation problem for the dataset array");
		return NULL;
	}
	
	for (int i = 0; i < numProcessors; i++) {
		#ifdef MOREVERBOSE
			info(stream, "Processor %d)", i);
		#endif
		datasetPtr[i] = isl_set_empty(isl_set_get_space(isl_set_from_union_set(isl_union_map_range(isl_union_map_copy(modifiedPolyhedralModelPtr[taskOnProcessor[i]] -> remappedMayReads)))));
		
		if (datasetPtr[i] == NULL) {
			error(stream, "Memory allocation problem for the dataset :(");
			return NULL;
		} 
		
		if (isl_union_map_is_empty(modifiedPolyhedralModelPtr[taskOnProcessor[i]] -> remappedMayReads) == isl_bool_false) {
			partialDatasetPtr = isl_union_set_apply(isl_union_set_copy(instantLocalSlicePtr[i]), isl_union_map_copy(modifiedPolyhedralModelPtr[taskOnProcessor[i]] -> remappedMayReads));
			
			if (isl_union_set_is_empty(partialDatasetPtr) == isl_bool_false)
				datasetPtr[i] = isl_set_union(datasetPtr[i], isl_set_from_union_set(partialDatasetPtr));
		}
		
		if (isl_union_map_is_empty(modifiedPolyhedralModelPtr[taskOnProcessor[i]] -> remappedMayWrites) == isl_bool_false) {
			partialDatasetPtr = isl_union_set_apply(isl_union_set_copy(instantLocalSlicePtr[i]), isl_union_map_copy(modifiedPolyhedralModelPtr[taskOnProcessor[i]] -> remappedMayWrites));
			
			if (isl_union_set_is_empty(partialDatasetPtr) == isl_bool_false)
				datasetPtr[i] = isl_set_union(datasetPtr[i], isl_set_from_union_set(partialDatasetPtr));
		}
		
		if (isl_union_map_is_empty(modifiedPolyhedralModelPtr[taskOnProcessor[i]] -> remappedMustWrites) == isl_bool_false) {
			partialDatasetPtr = isl_union_set_apply(isl_union_set_copy(instantLocalSlicePtr[i]), isl_union_map_copy(modifiedPolyhedralModelPtr[taskOnProcessor[i]] -> remappedMustWrites));
			
			if (isl_union_set_is_empty(partialDatasetPtr) == isl_bool_false)
				datasetPtr[i] = isl_set_union(datasetPtr[i], isl_set_from_union_set(partialDatasetPtr));
		}
		
		if (datasetPtr[i] == NULL) {
			error(stream, "Problem during dataset construction");
			return NULL;
		} 
		
		datasetPtr[i] = isl_set_coalesce(datasetPtr[i]);

		if (datasetPtr[i] == NULL) {
			error(stream, "Problem during dataset finalization");
			return NULL;
		} 

		#ifdef MOREVERBOSE
			printer = isl_printer_to_file(isl_set_get_ctx(datasetPtr[i]), stream);
			
			if(printer == NULL) {
				error(stream, "Memory allocation problem :(");
				return NULL;
			} 
			
			printer = isl_printer_set_indent(printer, moreIndent);
			
			fprintf(stream, "Instant local dataset: ");
			printer = isl_printer_print_set(printer, datasetPtr[i]);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return NULL;
			} 
			
			fprintf(stream, "\n");
			fflush(stream);
		#endif

	}
	
	return datasetPtr;
}

isl_stat evaluate_fundamental_lattice (FILE * stream, isl_set * concurrentDatasetPtr, isl_set ** translatesPtr, unsigned numTranslates, unsigned long * costPtr) {
	// Pointer to the Z - polyhedron to be evaluated
	isl_set * zPolyhedron = NULL;
	// Maximum number of memory conflicts count for the current fundamental lattice
	unsigned cost = 0;
	// Parameters for the callback function
	set_cardinality_params * cardParams = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	#ifdef MOREVERBOSE
		// Pointer to the printer
		isl_printer * printer = NULL;
	#endif
	
	
	for (unsigned i = 0; i < numTranslates; i++) {
		zPolyhedron = isl_set_intersect(isl_set_copy(concurrentDatasetPtr), isl_set_copy(translatesPtr[i]));
		
		if (zPolyhedron == NULL) {
			error(stream, "Error during building the Z - polyhedron");
			return isl_stat_error;
		}
		
		#ifdef MOREVERBOSE
			fprintf(stream, "Z - polyhedron of the points in the translate %u:\n", i);
			
			printer = isl_printer_to_file(isl_set_get_ctx(concurrentDatasetPtr), stream);
			
			if(printer == NULL) {
				error(stream, "Memory allocation problem :(");
				return isl_stat_error;
			}
			
			printer = isl_printer_set_indent(printer, moreIndent);
			
			printer = isl_printer_print_set(printer, zPolyhedron);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\n");
			fflush(stream);
		#endif
		
		cardParams = malloc(sizeof(set_cardinality_params));
	
		if (cardParams == NULL)
			return isl_stat_error;
	
		cardParams -> count = 0;
	
		outcome = isl_set_foreach_point (zPolyhedron, set_cardinality, (void *)cardParams);
	
		if (outcome == isl_stat_error) {
			error(stream, "Error during counting points in the Z - polyhedron");
			return isl_stat_error;
		}
		
		cardParams -> count;
		
		#ifdef MOREVERBOSE
			fprintf(stream, "Number of points: %u\n", cardParams -> count);
			fflush(stream);
		#endif
		
		if (cardParams -> count > cost)
			cost = cardParams -> count;
	}
	
	#ifdef VERBOSE
		fprintf(stream, "Cost function value for the current lattice and the current date: %u\n", cost);
		fflush(stream);
	#endif
	
	*costPtr += cost;
	
	return isl_stat_ok;
}


isl_stat access_matrix_fundamental_lattice (FILE * stream, isl_set ** instantLocalDatasetPtr, isl_set ** translatesPtr, unsigned numTranslates, unsigned numProcessors, unsigned ** accessMatrix) {
	// Pointer to the Z - polyhedron to be evaluated
	isl_set * zPolyhedron = NULL;
	// Parameters for the callback function
	set_cardinality_params * cardParams = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	#ifdef MOREVERBOSE
		// Pointer to the printer
		isl_printer * printer = NULL;
	#endif
	
	
	for (unsigned i = 0; i < numTranslates; i++)
		for (unsigned j = 0; j < numProcessors; j++) {
			zPolyhedron = isl_set_detect_equalities(isl_set_coalesce(isl_set_intersect(isl_set_copy(instantLocalDatasetPtr[j]), isl_set_copy(translatesPtr[i]))));
		
			if (zPolyhedron == NULL) {
				error(stream, "Error during building the Z - polyhedron");
				return isl_stat_error;
			}
		
		#ifdef MOREVERBOSE
			fprintf(stream, "Z - polyhedron of the points in the translate %u accessed by the processor %u:\n", i, j);
			
			printer = isl_printer_to_file(isl_set_get_ctx(zPolyhedron), stream);
			
			if(printer == NULL) {
				error(stream, "Memory allocation problem :(");
				return isl_stat_error;
			}
			
			printer = isl_printer_set_indent(printer, moreIndent);
			
			printer = isl_printer_print_set(printer, zPolyhedron);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\n");
			fflush(stream);
		#endif
		
		cardParams = malloc(sizeof(set_cardinality_params));
	
		if (cardParams == NULL)
			return isl_stat_error;
	
		cardParams -> count = 0;
	
		outcome = isl_set_foreach_point (zPolyhedron, set_cardinality, (void *)cardParams);
	
		if (outcome == isl_stat_error) {
			error(stream, "Error during counting points in the Z - polyhedron");
			return isl_stat_error;
		}
		
		accessMatrix[i][j] = cardParams -> count;
		
		#ifdef MOREVERBOSE
			fprintf(stream, "Number of points: %u\n", accessMatrix[i][j]);
			fflush(stream);
		#endif
		}
	
	return isl_stat_ok;
}
