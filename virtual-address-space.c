#ifdef MOREVERBOSE
	#define VERBOSE
#endif
/*
 * Implementation virtual allocation function(s)
 */ 

#include<isl/set.h>
#include<isl/constraint.h>

#include "config.h"
#include "support.h"
#include "model.h"
#include "partitioning.h"

// Dimensions added by the mapping policy
const unsigned dPolicy = 1;

isl_stat virtual_allocation (FILE * stream, isl_ctx * optionsHdl, pet_scop ** polyhedralModelPtr, manipulated_polyhedral_model ** modifiedPolyhedralModel, unsigned numTasks, unsigned * dimPtr) {
	
	// Maximum dimension of the arrays to be allocated
	unsigned dMax = 0;
	// Dimension of the padding zeros for the address space currently being reallocated
	unsigned dZeros = 0;
	// Array of dimensions of each task
	unsigned * dTask = NULL;
	#ifdef VERBOSE
		// Pointer to the printer
		isl_printer * printer = NULL;
	#endif
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
	
	if (dTask == NULL) {
		error(stream, "Memory allocation problem :(");
		return isl_stat_error;
	} 
	
	#ifdef VERBOSE
		printer = isl_printer_to_file(optionsHdl, stream);
		
		if(printer == NULL) {
			error(stream, "Memory allocation problem :(");
			return isl_stat_error;
		} 
	#endif
	
	// 1) We determine the dimension of the allocation address space
	for (int i = 0; i < numTasks; i++) {
		// Here we assume that each task has only one array
		originalArrayPtr = isl_set_copy(polyhedralModelPtr[i] -> arrays[0] -> extent);
		
		if (originalArrayPtr == NULL) {
			error(stream, "Cannot retrieve the original address space");
			free(dTask);
			return isl_stat_error;
		} 
		
		dTask[i] = isl_set_dim(originalArrayPtr, isl_dim_set);
		
		#ifdef VERBOSE
			fprintf(stream, "Original array of task %d: ", i);
			fflush(stream);
			printer = isl_printer_print_set(printer, originalArrayPtr);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\nDimensionality: %d\n", dTask[i]);
		#endif
	}
		
	// Maximum algorithm
	dMax = dTask[0];
	
	for (int i = 1; i < numTasks; i++)
		if (dMax < dTask[i])
			dMax = dTask[i];
	
	*dimPtr = dMax + dPolicy;
	#ifdef VERBOSE
		fprintf(stream, "Maximum dimensionality of the arrays to be allocated: %d\n", dMax);
		fprintf(stream, "Dimensionality of the allocation address space: %d\n", *dimPtr);
	#endif	
		
	for (int i = 0; i < numTasks; i++) {
		dZeros = *dimPtr - dTask[i] - dPolicy;
		
		#ifdef VERBOSE
			info(stream, "Task %d)", i);
			fprintf(stream, "Padding zeros: %u\n", dZeros);
			isl_printer_set_indent(printer, moreIndent);
		#endif
		
		originalArrayPtr = isl_set_copy(polyhedralModelPtr[i] -> arrays[0] -> extent);
		// 2) We build the allocation relation
		// Building the allocation address space set
		allocationArraySpacePtr = isl_space_set_alloc(optionsHdl, 0, *dimPtr);
		
		if (allocationArraySpacePtr == NULL)
			return isl_stat_error;
		
//		allocationArraySpacePtr = isl_space_set_tuple_name(allocationArraySpacePtr, isl_dim_set, "V");
		
		if (allocationArraySpacePtr == NULL)
			return isl_stat_error;
		
		// Building the allocation address space 
		allocationArrayPtr = isl_set_universe(allocationArraySpacePtr);
		
		if (allocationArrayPtr == NULL)
			return isl_stat_error;
		
		#ifdef VERBOSE
			fprintf(stream, "Allocated set: ");
			fflush(stream);
			printer = isl_printer_print_set(printer, allocationArrayPtr);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\n");
		#endif
		
		allocationRelationPtr = isl_map_from_domain_and_range(isl_set_copy(originalArrayPtr), isl_set_copy(allocationArrayPtr));
		
		if (allocationRelationPtr == NULL)
			return isl_stat_error;
		
		#ifdef MOREVERBOSE
			fprintf(stream, "Unconstrained relation: ");
			fflush(stream);
			printer = isl_printer_print_map(printer, allocationRelationPtr);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\n");
		#endif
		
		allocationRelationLocalSpacePtr = isl_local_space_from_space(isl_map_get_space(allocationRelationPtr));
		
		if (allocationRelationLocalSpacePtr == NULL)
			return isl_stat_error;
		
		#ifdef MOREVERBOSE
			fprintf(stream, "Local space to constrain: ");
			fflush(stream);
			printer = isl_printer_print_local_space(printer, allocationRelationLocalSpacePtr);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\n");
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
			fprintf(stream, "Constraints of the virtual allocation policy: ");
			fflush(stream);
			printer = isl_printer_print_map(printer, allocationRelationPtr);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\n");
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
		for (int j = dPolicy + dTask[i]; j < *dimPtr; j++) {
			constraintPtr = isl_constraint_alloc_equality(isl_local_space_copy(allocationRelationLocalSpacePtr));
		
			if (constraintPtr == NULL)
				return isl_stat_error;
		
			constraintPtr = isl_constraint_set_coefficient_si(constraintPtr, isl_dim_out, j, 1);
			constraintPtr = isl_constraint_set_constant_si(constraintPtr, 0);
			allocationRelationPtr = isl_map_add_constraint(allocationRelationPtr, constraintPtr);
		}
		
		#ifdef VERBOSE
			fprintf(stream, "Virtual address space allocation: ");
			fflush(stream);
			isl_printer_print_map(printer, allocationRelationPtr);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\n");
		#endif
		// May - reads remapping
		modifiedPolyhedralModel[i] -> remappedMayReads = isl_union_map_apply_range (pet_scop_get_may_reads(polyhedralModelPtr[i]), isl_union_map_from_map(isl_map_copy(allocationRelationPtr)));
		
		if (modifiedPolyhedralModel[i] -> remappedMayReads == NULL)
			return isl_stat_error;
		
		#ifdef VERBOSE
			fprintf(stream, "Original may - read access relation: ");
			fflush(stream);
			printer = isl_printer_print_union_map(printer, pet_scop_get_may_reads(polyhedralModelPtr[i]));
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\nRemapped may - read access relation: ");
			fflush(stream);
			printer = isl_printer_print_union_map(printer, modifiedPolyhedralModel[i] -> remappedMayReads);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\n");
		#endif
		// May - writes remapping
		modifiedPolyhedralModel[i] -> remappedMayWrites = isl_union_map_apply_range (pet_scop_get_may_writes(polyhedralModelPtr[i]), isl_union_map_from_map(isl_map_copy(allocationRelationPtr)));
		
		if (modifiedPolyhedralModel[i] -> remappedMayWrites == NULL)
			return isl_stat_error;
		
		#ifdef VERBOSE
			fprintf(stream, "Original may - write access relation: ");
			fflush(stream);
			printer = isl_printer_print_union_map(printer, pet_scop_get_may_writes(polyhedralModelPtr[i]));
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\nRemapped may - write access relation: ");
			fflush(stream);
			printer = isl_printer_print_union_map(printer, modifiedPolyhedralModel[i] -> remappedMayWrites);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\n");
		#endif
		// Must - writes remapping
		modifiedPolyhedralModel[i] -> remappedMustWrites = isl_union_map_apply_range (pet_scop_get_must_writes(polyhedralModelPtr[i]), isl_union_map_from_map(isl_map_copy(allocationRelationPtr)));
		
		if (modifiedPolyhedralModel[i] -> remappedMustWrites == NULL)
			return isl_stat_error;
		
		#ifdef VERBOSE
			fprintf(stream, "Original must - write access relation: ");
			fflush(stream);
			printer = isl_printer_print_union_map(printer, pet_scop_get_must_writes(polyhedralModelPtr[i]));
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\nRemapped must - write access relation: ");
			fflush(stream);
			printer = isl_printer_print_union_map(printer, modifiedPolyhedralModel[i] -> remappedMustWrites);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\n");
			isl_printer_set_indent(printer, lessIndent);
		#endif
		
	}
	
	#ifdef VERBOSE
		isl_printer_free(printer);
	#endif
	
	free(dTask);
	return isl_stat_ok;
	
}
