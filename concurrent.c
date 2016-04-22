#ifdef MOREVERBOSE
#define VERBOSE
#endif

#include<stdlib.h>

#include<isl/union_set.h>
//#include<barvinok/isl.h>

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
		
//		free(linearizationParams);
		
#ifdef VERBOSE
		fprintf(stream, "Linearized schedule:\n");
		printer = isl_printer_print_union_map(printer, modifiedPolyhedralModelPtr[i] -> linearizedSchedule);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
		fflush(stream);
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
	
#ifdef MOREVERBOSE
	fprintf(stream, "Cardinality of the set: %d\n", cardParams -> count);
//	fprintf(stream, "Barvinok cardinality:\n");
//	fflush(stream);
//	isl_pw_qpolynomial_dump(isl_set_card(isl_set_from_union_set(lexLtSetPtr)));
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

isl_set * concurrent_dataset_build(FILE * stream, manipulated_polyhedral_model ** modifiedPolyhedralModelPtr, isl_union_set ** polyhedralSlicePtr, unsigned numTasks) {
	// Array of the datasets of each task
	isl_set ** datasetPtr = NULL;
	// Pointer to a part of the dataset of the current task, or at the current concurrent dataset
	isl_set * partialDatasetPtr = NULL;
#ifdef MOREVERBOSE
	// Pointer to the printer
	isl_printer * printer = NULL;
#endif
	
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
			partialDatasetPtr = isl_set_from_union_set(isl_union_set_apply(isl_union_set_copy(polyhedralSlicePtr[i]), isl_union_map_copy(modifiedPolyhedralModelPtr[i] -> remappedMayReads)));
			
			datasetPtr[i] = isl_set_union(datasetPtr[i], partialDatasetPtr);
		}
		
		if (isl_union_map_is_empty(modifiedPolyhedralModelPtr[i] -> remappedMayWrites) == isl_bool_false) {
			partialDatasetPtr = isl_set_from_union_set(isl_union_set_apply(isl_union_set_copy(polyhedralSlicePtr[i]), isl_union_map_copy(modifiedPolyhedralModelPtr[i] -> remappedMayWrites)));
			
			datasetPtr[i] = isl_set_union(datasetPtr[i], partialDatasetPtr);
		}
		
		if (isl_union_map_is_empty(modifiedPolyhedralModelPtr[i] -> remappedMustWrites) == isl_bool_false) {
			partialDatasetPtr = isl_set_from_union_set(isl_union_set_apply(isl_union_set_copy(polyhedralSlicePtr[i]), isl_union_map_copy(modifiedPolyhedralModelPtr[i] -> remappedMustWrites)));
		
			datasetPtr[i] = isl_set_union(datasetPtr[i], partialDatasetPtr);
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
	
	partialDatasetPtr = datasetPtr[0];
	
	for (int i = 1; i < numTasks; i++)
		partialDatasetPtr = isl_set_union(partialDatasetPtr, datasetPtr[i]);
	
	return isl_set_coalesce(partialDatasetPtr);
}