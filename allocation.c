#ifdef MOREVERBOSE
#define VERBOSE
#endif
/*
 * Implementation virtual allocation function(s)
 */ 

#include<isl/set.h>
#include<isl/constraint.h>

#include "partitioning.h"

// Dimensions added by the mapping policy
const unsigned dPolicy = 1;

isl_stat virtual_allocation (isl_ctx * optionsHdl, pet_scop ** polyhedralModelPtr, remapped_access_relations * remappedAccessRelations, unsigned numTasks) {
	
	// Dimension of the allocation address space
	unsigned dAllocation = 0;
	// Maximum dimension of the arrays to be allocated
	unsigned dMax = 0;
	// Dimension of the padding zeros for the address space currently being reallocated
	unsigned dZeros = 0;
	// Array of dimensions of each task
	unsigned * dTask = NULL;
	// Pointer to the array to be allocated, for the current task
	isl_set * originalArrayPtr = NULL;
	// Pointer to the space of the allocation address space, for the current task
	isl_space * allocationArraySpacePtr = NULL;
	// Pointer to the space of the allocation address space, for the current task
	isl_set * allocationArrayPtr = NULL;
	// Pointer to the virtual address space allocation
	isl_map * allocationRelationPtr = NULL;
	// Pointer to the local space of the virtual address space allocation
	isl_local_space * allocationRelationLocalSpacePtr = NULL;
	// Pointer to the constraint under building
	isl_constraint * constraintPtr = NULL;
	
	dTask = malloc(numTasks * sizeof(unsigned));
	
	// 1) We determine the dimension of the allocation address space
	for (int i = 0; i < numTasks; i++) {
		// Here we assume that each task has only one array
		originalArrayPtr = isl_set_copy(polyhedralModelPtr[i] -> arrays[0] -> extent);
		
		if (originalArrayPtr == NULL) {
			free(dTask);
			return isl_stat_error;
		}
		
		dTask[i] = isl_set_dim(originalArrayPtr, isl_dim_set);
		
#ifdef VERBOSE
		printf("Original array of task %d: ", i);
		fflush(stdout);
		isl_set_dump(originalArrayPtr);
		printf("Dimensionality: %d\n", dTask[i]);
#endif
	}
		
	// Maximum algorithm
	dMax = dTask[0];
	
	for (int i = 1; i < numTasks; i++)
		if (dMax < dTask[i])
			dMax = dTask[i];
	
	dAllocation = dMax + dPolicy;
#ifdef VERBOSE
	printf("Maximum dimensionality of the arrays to be allocated: %d\n", dMax);	
	printf("Dimensionality of the allocation address space: %d\n", dAllocation);
#endif	
		
	for (int i = 0; i < numTasks; i++) {
		dZeros = dAllocation - dTask[i] - dPolicy;
		
#ifdef VERBOSE
		printf("Task %d\n", i);
		printf("Padding zeros: %u\n", dZeros);
#endif
		
		originalArrayPtr = isl_set_copy(polyhedralModelPtr[i] -> arrays[0] -> extent);
		// 2) We build the allocation relation
		// Building the allocation address space set
		allocationArraySpacePtr = isl_space_set_alloc(optionsHdl, 0, dAllocation);
		
		if (allocationArraySpacePtr == NULL)
			return isl_stat_error;
		
		allocationArraySpacePtr = isl_space_set_tuple_name(allocationArraySpacePtr, isl_dim_set, "V");
		
		if (allocationArraySpacePtr == NULL)
			return isl_stat_error;
		
		// Building the allocation address space 
		allocationArrayPtr = isl_set_universe(allocationArraySpacePtr);
		
		if (allocationArrayPtr == NULL)
			return isl_stat_error;
		
#ifdef VERBOSE
		printf("Allocated set: ");
		fflush(stdout);
		isl_set_dump(allocationArrayPtr);
#endif
		
		allocationRelationPtr = isl_map_from_domain_and_range(isl_set_copy(originalArrayPtr), isl_set_copy(allocationArrayPtr));
		
		if (allocationRelationPtr == NULL)
			return isl_stat_error;
		
#ifdef MOREVERBOSE
		printf("Unconstrained relation: ");
		fflush(stdout);
		isl_map_dump(allocationRelationPtr);
#endif
		
		allocationRelationLocalSpacePtr = isl_local_space_from_space(isl_map_get_space(allocationRelationPtr));
		
		if (allocationRelationLocalSpacePtr == NULL)
			return isl_stat_error;
		
#ifdef MOREVERBOSE
		printf("Local space to constrain: ");
		fflush(stdout);
		isl_local_space_dump(allocationRelationLocalSpacePtr);
#endif
		
		// Constraint on the dimension(s) used by the virtual allocation policy
		constraintPtr = isl_constraint_alloc_equality(isl_local_space_copy(allocationRelationLocalSpacePtr));
		
		if (constraintPtr == NULL)
			return isl_stat_error;
		
		// The first dimension of the allocation space must be equal to the task number
		constraintPtr = isl_constraint_set_coefficient_si(constraintPtr, isl_dim_out, 0, 1);
		constraintPtr = isl_constraint_set_constant_si(constraintPtr, -i);
		allocationRelationPtr = isl_map_add_constraint(allocationRelationPtr, constraintPtr);
		
#ifdef MOREVERBOSE
		printf("Constraints of the virtual allocation policy: ");
		fflush(stdout);
		isl_map_dump(allocationRelationPtr);
#endif
		
		// Constraints on the original dimensions
		for (int j = 0; j < dTask[i]; j++) {
			constraintPtr = isl_constraint_alloc_equality(isl_local_space_copy(allocationRelationLocalSpacePtr));
		
			if (constraintPtr == NULL)
				return isl_stat_error;
		
			constraintPtr = isl_constraint_set_coefficient_si(constraintPtr, isl_dim_in, j, 1);
			constraintPtr = isl_constraint_set_coefficient_si(constraintPtr, isl_dim_out, dPolicy + j, -1);
			allocationRelationPtr = isl_map_add_constraint(allocationRelationPtr, constraintPtr);
		}
		
		// Constraint on the padding zeros
		for (int j = dPolicy + dTask[i]; j < dAllocation; j++) {
			constraintPtr = isl_constraint_alloc_equality(isl_local_space_copy(allocationRelationLocalSpacePtr));
		
			if (constraintPtr == NULL)
				return isl_stat_error;
		
			constraintPtr = isl_constraint_set_coefficient_si(constraintPtr, isl_dim_out, j, 1);
			constraintPtr = isl_constraint_set_constant_si(constraintPtr, 0);
			allocationRelationPtr = isl_map_add_constraint(allocationRelationPtr, constraintPtr);
		}
		
#ifdef VERBOSE
		printf("Virtual address space allocation: ");
		fflush(stdout);
		isl_map_dump(allocationRelationPtr);
#endif
		// May - reads remapping
		remappedAccessRelations[i].remappedMayReads = isl_union_map_apply_range (pet_scop_get_may_reads(polyhedralModelPtr[i]), isl_union_map_from_map(isl_map_copy(allocationRelationPtr)));
		
		if (remappedAccessRelations[i].remappedMayReads == NULL)
			return isl_stat_error;
		
#ifdef VERBOSE
		printf("Original may - read access relation: ");
		fflush(stdout);
		isl_union_map_dump(pet_scop_get_may_reads(polyhedralModelPtr[i]));
		printf("Remapped may - read access relation: ");
		fflush(stdout);
		isl_union_map_dump(remappedAccessRelations[i].remappedMayReads);
#endif
		// May - writes remapping
		remappedAccessRelations[i].remappedMayWrites = isl_union_map_apply_range (pet_scop_get_may_writes(polyhedralModelPtr[i]), isl_union_map_from_map(isl_map_copy(allocationRelationPtr)));
		
		if (remappedAccessRelations[i].remappedMayWrites == NULL)
			return isl_stat_error;
		
#ifdef VERBOSE
		printf("Original may - write access relation: ");
		fflush(stdout);
		isl_union_map_dump(pet_scop_get_may_writes(polyhedralModelPtr[i]));
		printf("Remapped may - write access relation: ");
		fflush(stdout);
		isl_union_map_dump(remappedAccessRelations[i].remappedMayWrites);
#endif
		// Must - writes remapping
		remappedAccessRelations[i].remappedMustWrites = isl_union_map_apply_range (pet_scop_get_must_writes(polyhedralModelPtr[i]), isl_union_map_from_map(isl_map_copy(allocationRelationPtr)));
		
		if (remappedAccessRelations[i].remappedMustWrites == NULL)
			return isl_stat_error;
		
#ifdef VERBOSE
		printf("Original must - write access relation: ");
		fflush(stdout);
		isl_union_map_dump(pet_scop_get_must_writes(polyhedralModelPtr[i]));
		printf("Remapped must - write access relation: ");
		fflush(stdout);
		isl_union_map_dump(remappedAccessRelations[i].remappedMustWrites);
#endif
		
	}
	
	free(dTask);
	return isl_stat_ok;
	
}
