#ifdef MOREVERBOSE
#define VERBOSE
#endif
/*
 * Implementation of the function related to the elimination of the parameters
 * from the representation
 */
#include<stdio.h>

#include<isl/constraint.h>

#include "config.h"
#include "support.h"
#include "partitioning.h"

typedef struct {
	isl_union_map * bounded;
	unsigned taskNum;
} add_parameter_constraint_params;

isl_stat eliminate_parameters_map(isl_union_map **, unsigned);
isl_stat add_parameter_constraint (isl_map *, void *);

isl_stat eliminate_parameters (manipulated_polyhedral_model * modifiedPolyhedralModel, unsigned numTasks) {
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	
	for (int i = 0; i < numTasks; i++) {
#ifdef VERBOSE
		info("Task %d)", i);
#endif
		outcome = eliminate_parameters_map (&(modifiedPolyhedralModel[i].flattenedSchedule), i);
		
		if (outcome == isl_stat_error)
			return isl_stat_error;
		
#ifdef VERBOSE
		printf("Constrained flattened schedule:\n");
		fflush(stdout);
		isl_union_map_dump(modifiedPolyhedralModel[i].flattenedSchedule);
#endif
		
		outcome = eliminate_parameters_map (&(modifiedPolyhedralModel[i].remappedMayReads), i);
		
		if (outcome == isl_stat_error)
			return isl_stat_error;
		
#ifdef VERBOSE
		printf("Constrained may reads:\n");
		fflush(stdout);
		isl_union_map_dump(modifiedPolyhedralModel[i].remappedMayReads);
#endif
	
	}
	
	return isl_stat_ok;
	
}

isl_stat eliminate_parameters_map(isl_union_map ** umap, unsigned taskNum) {
	// Parameters for the callback function
	add_parameter_constraint_params * params = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	
	params = malloc(sizeof(add_parameter_constraint_params));
	
	if(params == NULL)
		return isl_stat_error;
	
	// Initialization of the parameters
	params -> bounded = NULL;
	params -> taskNum = taskNum;
	
	outcome = isl_union_map_foreach_map(*umap, add_parameter_constraint, params);
	
	if (outcome == isl_stat_error)
		return isl_stat_error;
	
	*umap = params -> bounded;
		
#ifdef MOREVERBOSE
	printf("Constrained union map:\n");
	fflush(stdout);
	isl_union_map_dump(*umap);
#endif
	
	free(params);
	return isl_stat_ok;

}


isl_stat add_parameter_constraint (isl_map * map, void * user) {
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
	
		parameterConstraintPtr = isl_constraint_alloc_equality(isl_local_space_copy(localSpacePtr ));
		
		if (parameterConstraintPtr == NULL)
			return isl_stat_error;
		
		parameterConstraintPtr = isl_constraint_set_coefficient_si(parameterConstraintPtr, isl_dim_param, i, 1);
		parameterConstraintPtr = isl_constraint_set_constant_si(parameterConstraintPtr, -4);
		map = isl_map_add_constraint(map, parameterConstraintPtr);
	}
	
	map = isl_map_project_out(map, isl_dim_param, 0, NUMPARAMS[params -> taskNum]);
	
#ifdef MOREVERBOSE
	printf("Map bounded:\n");
	fflush(stdout);
	isl_map_dump(map);
#endif
	
	// To pass out the modified map
	if (params -> bounded == NULL)
		params -> bounded = isl_union_map_from_map(map);
	else 
		params -> bounded = isl_union_map_union(params -> bounded, isl_union_map_from_map(map));
	
	return isl_stat_ok;
	
}
