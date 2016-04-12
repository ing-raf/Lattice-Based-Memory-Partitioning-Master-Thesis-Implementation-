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
} add_parameter_constraint_params;

isl_union_map * eliminate_parameters_map(isl_union_map *, unsigned);
isl_union_set * eliminate_parameters_set(isl_union_set *, unsigned);
isl_stat add_parameter_constraint_map (isl_map *, void *);
isl_stat add_parameter_constraint_set (isl_set *, void *);

isl_stat eliminate_parameters (pet_scop ** polyhedralModelPtr, manipulated_polyhedral_model ** modifiedPolyhedralModel, unsigned numTasks) {
	
	for (int i = 0; i < numTasks; i++) {
#ifdef VERBOSE
		info("Task %d)", i);
#endif
		
		modifiedPolyhedralModel[i] -> instanceSet = eliminate_parameters_set(pet_scop_get_instance_set(polyhedralModelPtr[i]), i);

		if (modifiedPolyhedralModel[i] -> instanceSet == NULL)
			return isl_stat_error;
		
#ifdef VERBOSE
		printf("Constrained instance set:\n");
		fflush(stdout);
		isl_union_set_dump(modifiedPolyhedralModel[i] ->  instanceSet);
#endif		
		
		modifiedPolyhedralModel[i] -> flattenedSchedule = eliminate_parameters_map (modifiedPolyhedralModel[i] -> flattenedSchedule, i);
		
		if (modifiedPolyhedralModel[i] -> flattenedSchedule == NULL)
			return isl_stat_error;
		
#ifdef VERBOSE
		printf("Constrained flattened schedule:\n");
		fflush(stdout);
		isl_union_map_dump(modifiedPolyhedralModel[i] -> flattenedSchedule);
#endif
		
		modifiedPolyhedralModel[i] -> remappedMayReads = eliminate_parameters_map (modifiedPolyhedralModel[i] -> remappedMayReads, i);
		
		if (modifiedPolyhedralModel[i] -> remappedMayReads == NULL)
			return isl_stat_error;
		
#ifdef VERBOSE
		printf("Constrained may reads:\n");
		fflush(stdout);
		isl_union_map_dump(modifiedPolyhedralModel[i] -> remappedMayReads);
#endif
		
#ifdef VERBOSE
		printf("May writes to constraint:\n");
		fflush(stdout);
		isl_union_map_dump(modifiedPolyhedralModel[i] -> remappedMayWrites);
#endif
		
		modifiedPolyhedralModel[i] -> remappedMayWrites = eliminate_parameters_map (modifiedPolyhedralModel[i] -> remappedMayWrites, i);
		
		if (modifiedPolyhedralModel[i] -> remappedMayWrites == NULL)
			return isl_stat_error;
		
#ifdef VERBOSE
		printf("Constrained may writes:\n");
		fflush(stdout);
		isl_union_map_dump(modifiedPolyhedralModel[i] -> remappedMayWrites);
#endif
		
		modifiedPolyhedralModel[i] -> remappedMustWrites = eliminate_parameters_map (modifiedPolyhedralModel[i] -> remappedMustWrites, i);
		
		if (modifiedPolyhedralModel[i] -> remappedMustWrites == NULL)
			return isl_stat_error;
		
#ifdef VERBOSE
		printf("Constrained must writes:\n");
		fflush(stdout);
		isl_union_map_dump(modifiedPolyhedralModel[i] -> remappedMustWrites);
#endif
	
	}
	
	return isl_stat_ok;
	
}

isl_union_map * eliminate_parameters_map(isl_union_map * umap, unsigned taskNum) {
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
	
	outcome = isl_union_map_foreach_map(umap, add_parameter_constraint_map, params);
	
	if (outcome == isl_stat_error)
		return NULL;
	
//	umap = params -> bounded;
	
#ifdef MOREVERBOSE
	printf("Constrained union map:\n");
	fflush(stdout);
	isl_union_map_dump(params -> bounded.umap);
#endif
	
	returnMap = params -> bounded.umap;
	free(params);
	
	if (returnMap != NULL)
		return returnMap;
	else { // builds an empty map in the same space of the given union map
		return isl_union_map_empty(isl_space_alloc(isl_union_map_get_ctx(umap), 0,0,0));
	}

}


isl_stat add_parameter_constraint_map (isl_map * map, void * user) {
	// Pointer to the input parameters
	add_parameter_constraint_params * params = (add_parameter_constraint_params *)user;
	// Pointer to the statement schedule local space
	isl_local_space * localSpacePtr = NULL;
	// Pointer to the constraint under building upon the parameters
	isl_constraint * parameterConstraintPtr = NULL;
	
#ifdef MOREVERBOSE
	printf("Map to bound:\n");
	fflush(stdout);
	isl_map_dump(map);
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
	printf("Map bounded:\n");
	fflush(stdout);
	isl_map_dump(map);
#endif
	
	// To pass out the modified map
	if (params -> bounded.umap == NULL)
		params -> bounded.umap = isl_union_map_from_map(map);
	else 
		params -> bounded.umap = isl_union_map_union(params -> bounded.umap, isl_union_map_from_map(map));
	
	return isl_stat_ok;
	
}

isl_union_set * eliminate_parameters_set(isl_union_set * uset, unsigned taskNum) {
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
	
	outcome = isl_union_set_foreach_set(uset, add_parameter_constraint_set, params);
	
	if (outcome == isl_stat_error)
		return NULL;
	
#ifdef MOREVERBOSE
	printf("Constrained union set:\n");
	fflush(stdout);
	isl_union_set_dump(params -> bounded.uset);
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
	// Pointer to the input parameters
	add_parameter_constraint_params * params = (add_parameter_constraint_params *)user;
	// Pointer to the statement schedule local space
	isl_local_space * localSpacePtr = NULL;
	// Pointer to the constraint under building upon the parameters
	isl_constraint * parameterConstraintPtr = NULL;
	
#ifdef MOREVERBOSE
	printf("Set to bound:\n");
	fflush(stdout);
	isl_set_dump(set);
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
	printf("Set bounded:\n");
	fflush(stdout);
	isl_set_dump(set);
#endif
	
	// To pass out the modified set
	if (params -> bounded.uset == NULL)
		params -> bounded.uset = isl_union_set_from_set(set);
	else 
		params -> bounded.uset = isl_union_set_union(params -> bounded.uset, isl_union_set_from_set(set));
	
	return isl_stat_ok;
	
}
