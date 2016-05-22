#ifdef MOREVERBOSE
#define VERBOSE
#endif
/*
 * Implementation of the function related to the elimination of the parameters
 * from the representation
 */
#include<stdio.h>

#include<pet.h>
#include<isl/constraint.h>
#include<isl/union_set.h>
#include<isl/union_map.h>

#include "config.h"
#include "support.h"
#include "partitioning.h"

typedef struct {
	union {
		isl_union_map * umap;
		isl_union_set * uset;
	} bounded;
	unsigned taskNum;
#ifdef MOREVERBOSE
	isl_printer * printer;
#endif
} add_parameter_constraint_params;

#ifndef MOREVERBOSE
isl_union_map * eliminate_parameters_map(isl_union_map *, unsigned);
isl_union_set * eliminate_parameters_set(isl_union_set *, unsigned);
#else
isl_union_map * eliminate_parameters_map(isl_printer *, isl_union_map *, unsigned);
isl_union_set * eliminate_parameters_set(isl_printer *, isl_union_set *, unsigned);
#endif
isl_stat add_parameter_constraint_map (isl_map *, void *);
isl_stat add_parameter_constraint_set (isl_set *, void *);

isl_stat eliminate_parameters (FILE * stream, pet_scop ** polyhedralModelPtr, manipulated_polyhedral_model ** modifiedPolyhedralModel, unsigned numTasks) {
	
#ifdef MOREVERBOSE
	// Pointer to the printer
	isl_printer * printer = NULL;
#endif
	
	for (int i = 0; i < numTasks; i++) {
#ifdef MOREVERBOSE
		info(stream, "Task %d)", i);
		
		printer = isl_printer_to_file(isl_union_set_get_ctx(pet_scop_get_instance_set(polyhedralModelPtr[i])), stream);
		
		if(printer == NULL) {
			error(stream, "Memory allocation problem :(");
			return isl_stat_error;
		} 
		
		isl_printer_set_indent(printer, moreIndent);
#endif
		
#ifndef MOREVERBOSE
		modifiedPolyhedralModel[i] -> instanceSet = eliminate_parameters_set(pet_scop_get_instance_set(polyhedralModelPtr[i]), i);
#else
		modifiedPolyhedralModel[i] -> instanceSet = eliminate_parameters_set(printer, pet_scop_get_instance_set(polyhedralModelPtr[i]), i);
#endif
		
		if (modifiedPolyhedralModel[i] -> instanceSet == NULL)
			return isl_stat_error;
		
#ifdef MOREVERBOSE
		fprintf(stream, "Constrained instance set:\n");
		fflush(stream);
		printer = isl_printer_print_union_set(printer, modifiedPolyhedralModel[i] ->  instanceSet);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
#endif
		
#ifndef MOREVERBOSE
		modifiedPolyhedralModel[i] -> flattenedSchedule = eliminate_parameters_map (modifiedPolyhedralModel[i] -> flattenedSchedule, i);
#else
		modifiedPolyhedralModel[i] -> flattenedSchedule = eliminate_parameters_map (printer, modifiedPolyhedralModel[i] -> flattenedSchedule, i);
#endif
		
		if (modifiedPolyhedralModel[i] -> flattenedSchedule == NULL)
			return isl_stat_error;
		
#ifdef MOREVERBOSE
		fprintf(stream, "Constrained flattened schedule:\n");
		fflush(stream);
		printer = isl_printer_print_union_map(printer, modifiedPolyhedralModel[i] -> flattenedSchedule);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
#endif

#ifndef MOREVERBOSE
		modifiedPolyhedralModel[i] -> allocation = eliminate_parameters_map (modifiedPolyhedralModel[i] -> allocation, i);
#else
		modifiedPolyhedralModel[i] -> allocation = eliminate_parameters_map (printer, modifiedPolyhedralModel[i] -> allocation, i);
#endif
		
		if (modifiedPolyhedralModel[i] -> allocation == NULL)
			return isl_stat_error;
		
#ifdef MOREVERBOSE
		fprintf(stream, "Constrained allocation:\n");
		fflush(stream);
		printer = isl_printer_print_union_map(printer, modifiedPolyhedralModel[i] -> allocation);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
#endif
		
#ifndef MOREVERBOSE
		modifiedPolyhedralModel[i] -> remappedMayReads = eliminate_parameters_map (modifiedPolyhedralModel[i] -> remappedMayReads, i);
#else
		modifiedPolyhedralModel[i] -> remappedMayReads = eliminate_parameters_map (printer, modifiedPolyhedralModel[i] -> remappedMayReads, i);
#endif
		
		if (modifiedPolyhedralModel[i] -> remappedMayReads == NULL)
			return isl_stat_error;
		
#ifdef MOREVERBOSE
		fprintf(stream, "Constrained may reads:\n");
		fflush(stream);
		printer = isl_printer_print_union_map(printer, modifiedPolyhedralModel[i] -> remappedMayReads);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
#endif
		
#ifndef MOREVERBOSE
		modifiedPolyhedralModel[i] -> remappedMayWrites = eliminate_parameters_map (modifiedPolyhedralModel[i] -> remappedMayWrites, i);
#else
		modifiedPolyhedralModel[i] -> remappedMayWrites = eliminate_parameters_map (printer, modifiedPolyhedralModel[i] -> remappedMayWrites, i);
#endif
		
		if (modifiedPolyhedralModel[i] -> remappedMayWrites == NULL)
			return isl_stat_error;
		
#ifdef MOREVERBOSE
		fprintf(stream, "Constrained may writes:\n");
		fflush(stream);
		printer = isl_printer_print_union_map(printer, modifiedPolyhedralModel[i] -> remappedMayWrites);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
#endif
		
#ifndef MOREVERBOSE
		modifiedPolyhedralModel[i] -> remappedMustWrites = eliminate_parameters_map (modifiedPolyhedralModel[i] -> remappedMustWrites, i);
#else
		modifiedPolyhedralModel[i] -> remappedMustWrites = eliminate_parameters_map (printer, modifiedPolyhedralModel[i] -> remappedMustWrites, i);
#endif
		
		if (modifiedPolyhedralModel[i] -> remappedMustWrites == NULL)
			return isl_stat_error;
		
#ifdef MOREVERBOSE
		fprintf(stream, "Constrained must writes:\n");
		fflush(stream);
		printer = isl_printer_print_union_map(printer, modifiedPolyhedralModel[i] -> remappedMustWrites);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
		
		isl_printer_free(printer);
#endif
	
	}
	
	return isl_stat_ok;
	
}

#ifndef MOREVERBOSE
isl_union_map * eliminate_parameters_map(isl_union_map * umap, unsigned taskNum) {
#else
isl_union_map * eliminate_parameters_map(isl_printer * printer, isl_union_map * umap, unsigned taskNum) {
	// Handle to the output stream
	FILE * stream = NULL;
#endif
	// Return value
	isl_union_map * returnMap = NULL;
	// Parameters for the callback function
	add_parameter_constraint_params * params = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	
	params = malloc(sizeof(add_parameter_constraint_params));
	
	if(params == NULL)
		return NULL;
	
	// Initialization of the parameters
	params -> bounded.umap = NULL;
	params -> taskNum = taskNum;	
#ifdef MOREVERBOSE
	params -> printer = printer; 
#endif
	
	outcome = isl_union_map_foreach_map(umap, add_parameter_constraint_map, params);
	
	if (outcome == isl_stat_error)
		return NULL;
	
	if (params -> bounded.umap != NULL)
		returnMap = params -> bounded.umap;
	else // builds an empty map in the same space of the given union map
		returnMap = isl_union_map_empty(isl_space_alloc(isl_union_map_get_ctx(umap), 0,0,0));
		
	free(params);
	
#ifdef MOREVERBOSE
	stream = isl_printer_get_file(printer);
	fprintf(stream, "Constrained union map:\n");
	fflush(stream);
	printer = isl_printer_print_union_map(printer, returnMap);
	
	if(printer == NULL) {
		error(stream, "Printing problem :(");
		return NULL;
	} 
	
	fprintf(stream, "\n");
#endif
	return returnMap;
}


isl_stat add_parameter_constraint_map (isl_map * map, void * user) {
	// Pointer to the input parameters
	add_parameter_constraint_params * params = (add_parameter_constraint_params *)user;
#ifdef MOREVERBOSE
	// Handle to the output stream
	FILE * stream = NULL;
#endif
	// Pointer to the statement schedule local space
	isl_local_space * localSpacePtr = NULL;
	// Pointer to the constraint under building upon the parameters
	isl_constraint * parameterConstraintPtr = NULL;
	
#ifdef MOREVERBOSE
	stream = isl_printer_get_file(params -> printer);
	fprintf(stream, "Map to bound:\n");
	fflush(stream);
	params -> printer = isl_printer_print_map(params -> printer, map);
	
	if(params -> printer == NULL) {
		error(stream, "Printing problem :(");
		return isl_stat_error;
	} 
	
	fprintf(stream, "\n");
#endif
	
	localSpacePtr = isl_local_space_from_space(isl_map_get_space(map));
	
	if (localSpacePtr == NULL)
		return isl_stat_error;
	
	for (int i = 0; i < NUMPARAMS[params -> taskNum]; i++) {
	
		parameterConstraintPtr = isl_constraint_alloc_equality(isl_local_space_copy(localSpacePtr));
		
		if (parameterConstraintPtr == NULL)
			return isl_stat_error;
		
		parameterConstraintPtr = isl_constraint_set_coefficient_si(parameterConstraintPtr, isl_dim_param, i, 1);
		parameterConstraintPtr = isl_constraint_set_constant_si(parameterConstraintPtr, -PARAMS[params -> taskNum][i]);
		map = isl_map_add_constraint(map, parameterConstraintPtr);
	}
	
	map = isl_map_project_out(map, isl_dim_param, 0, NUMPARAMS[params -> taskNum]);
	
#ifdef MOREVERBOSE
	fprintf(stream, "Map bounded:\n");
	fflush(stream);
	params -> printer = isl_printer_print_map(params -> printer, map);
	
	if(params -> printer == NULL) {
		error(stream, "Printing problem :(");
		return isl_stat_error;
	} 
	
	fprintf(stream, "\n");
#endif
	
	// To pass out the modified map
	if (params -> bounded.umap == NULL)
		params -> bounded.umap = isl_union_map_from_map(map);
	else 
		params -> bounded.umap = isl_union_map_union(params -> bounded.umap, isl_union_map_from_map(map));
	
	return isl_stat_ok;
	
}

#ifndef MOREVERBOSE
isl_union_set * eliminate_parameters_set(isl_union_set * uset, unsigned taskNum) {
#else
isl_union_set * eliminate_parameters_set(isl_printer * printer, isl_union_set * uset, unsigned taskNum) {
	// Handle to the output stream
	FILE * stream = NULL;
#endif
	// Return value
	isl_union_set * returnSet = NULL;
	// Parameters for the callback function
	add_parameter_constraint_params * params = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	
	params = malloc(sizeof(add_parameter_constraint_params));
	
	if(params == NULL)
		return NULL;
	
	// Initialization of the parameters
	params -> bounded.uset = NULL;
	params -> taskNum = taskNum;
	
#ifdef MOREVERBOSE
	params -> printer = printer;
#endif
	
	outcome = isl_union_set_foreach_set(uset, add_parameter_constraint_set, params);
	
	if (outcome == isl_stat_error)
		return NULL;
	
#ifdef MOREVERBOSE
	stream = isl_printer_get_file(printer);
	fprintf(stream, "Constrained union set:\n");
	fflush(stream);
	printer = isl_printer_print_union_set(printer, params -> bounded.uset);
	
	if(printer == NULL) {
		error(stream, "Printing problem :(");
		return NULL;
	}
	
	fprintf(stream, "\n");
#endif
	
	returnSet = params -> bounded.uset;
	free(params);
	
	if (returnSet != NULL)
		return returnSet;
	else { // builds an empty map in the same space of the given union map
		return isl_union_set_empty(isl_space_set_alloc(isl_union_set_get_ctx(uset), 0,0));
	}

}


isl_stat add_parameter_constraint_set (isl_set * set, void * user) {
#ifdef MOREVERBOSE
	// Handle to the output stream
	FILE * stream = NULL;
#endif
	// Pointer to the input parameters
	add_parameter_constraint_params * params = (add_parameter_constraint_params *)user;
	// Pointer to the statement schedule local space
	isl_local_space * localSpacePtr = NULL;
	// Pointer to the constraint under building upon the parameters
	isl_constraint * parameterConstraintPtr = NULL;
	
#ifdef MOREVERBOSE
	stream = isl_printer_get_file(params -> printer);
	fprintf(stream, "Set to bound:\n");
	fflush(stream);
	params -> printer = isl_printer_print_set(params -> printer, set);
	
	if(params -> printer == NULL) {
		error(stream, "Printing problem :(");
		return isl_stat_error;
	} 
	
	fprintf(stream, "\n");
#endif
	
	localSpacePtr = isl_local_space_from_space(isl_set_get_space(set));
	
	if (localSpacePtr == NULL)
		return isl_stat_error;
	
	for (int i = 0; i < NUMPARAMS[params -> taskNum]; i++) {
	
		parameterConstraintPtr = isl_constraint_alloc_equality(isl_local_space_copy(localSpacePtr));
		
		if (parameterConstraintPtr == NULL)
			return isl_stat_error;
		
		parameterConstraintPtr = isl_constraint_set_coefficient_si(parameterConstraintPtr, isl_dim_param, i, 1);
		parameterConstraintPtr = isl_constraint_set_constant_si(parameterConstraintPtr, -PARAMS[params -> taskNum][i]);
		set = isl_set_add_constraint(set, parameterConstraintPtr);
	}
	
	set = isl_set_project_out(set, isl_dim_param, 0, NUMPARAMS[params -> taskNum]);
	
#ifdef MOREVERBOSE
	fprintf(stream, "Set bounded:\n");
	fflush(stream);
	params -> printer = isl_printer_print_set(params -> printer, set);
	
	if(params -> printer == NULL) {
		error(stream, "Printing problem :(");
		return isl_stat_error;
	} 
	
	fprintf(stream, "\n");
#endif
	
	// To pass out the modified set
	if (params -> bounded.uset == NULL)
		params -> bounded.uset = isl_union_set_from_set(set);
	else 
		params -> bounded.uset = isl_union_set_union(params -> bounded.uset, isl_union_set_from_set(set));
	
	return isl_stat_ok;
	
}
