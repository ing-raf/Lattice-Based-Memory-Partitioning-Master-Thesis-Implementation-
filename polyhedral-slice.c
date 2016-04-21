#ifdef MOREVERBOSE
#define VERBOSE
#endif
/*
 * Implementation of functions used to build the polyhedral slice
 */ 
#include<isl/union_set.h>
#include<isl/schedule_node.h>

#include "config.h"
#include "support.h"
#include "model.h"
#include "partitioning.h"

#define GO_ON isl_bool_true
#define STOP isl_bool_false

isl_bool findOutermostParallel (__isl_keep isl_schedule_node *, void *);

isl_stat physical_schedule (FILE * stream, isl_ctx * optionsHdl, pet_scop ** polyhedralModelPtr, manipulated_polyhedral_model ** modifiedPolyhedralModelPtr, unsigned numTasks) {
	// Dimensionality of the domain of the schedule of the current task
	unsigned scheduleDim = 0;
	// Depth of the parallel dimension of the current task
	int parallelIteratorPos = -1;
#ifdef VERBOSE
	// Pointer to the printer
	isl_printer * printer = NULL;
#endif
	// Pointer to the schedule tree of the current task
	isl_schedule * scheduleTreePtr = NULL;
	// Pointer to the original schedule of the current task
	isl_union_map * schedulePtr = NULL;
	// Pointer to the space of the domain of the schedule of the current task
	isl_space * scheduleSpacePtr = NULL;
	// Pointer to the local space of the domain of the schedule of the current task
	isl_local_space * scheduleLocalSpacePtr = NULL;
	// Pointer to the physical schedule as a multiple function
	isl_multi_aff * physicalScheduleFunctionPtr = NULL;
	// Pointer to the physical schedule dimension currently build
	isl_aff * physicalScheduleDimensionPtr = NULL;
	
	for (int i = 0; i < numTasks; i++) {
		scheduleTreePtr = pet_scop_get_schedule(polyhedralModelPtr[i]);
		
		if (scheduleTreePtr == NULL)
			return isl_stat_error;
	
		parallelIteratorPos = -1;
		isl_schedule_foreach_schedule_node_top_down(scheduleTreePtr, findOutermostParallel, (void *)(&parallelIteratorPos) );
		
#ifdef VERBOSE
		info(stream, "Task %d)", i);
		fprintf(stream, "Outermost parallel dimension found: %i\n", parallelIteratorPos);
		
		printer = isl_printer_to_file(optionsHdl, stream);
		
		if(printer == NULL) {
			error(stream, "Memory allocation problem :(");
			return isl_stat_error;
		} 
		
		isl_printer_set_indent(printer, moreIndent);
#endif
		
		schedulePtr = isl_schedule_get_map(scheduleTreePtr);
		
#ifdef VERBOSE
		fprintf(stream, "Flattened schedule:\n");
		fflush(stream);
		printer = isl_printer_print_union_map(printer, schedulePtr);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
#endif
		scheduleSpacePtr = isl_set_get_space(isl_set_from_union_set(isl_union_map_range(isl_union_map_copy(schedulePtr))));
		
		if (scheduleSpacePtr == NULL)
			return isl_stat_error;	
		
		scheduleLocalSpacePtr = isl_local_space_from_space(isl_space_copy(scheduleSpacePtr));
		
		if (scheduleLocalSpacePtr == NULL)
			return isl_stat_error;
		
		scheduleDim = isl_local_space_dim(scheduleLocalSpacePtr, isl_dim_set);
		
#ifdef VERBOSE
		fprintf(stream, "Dimensionality of the schedule domain: %d\n", scheduleDim);
		fflush(stream);
		printer = isl_printer_print_local_space(printer, scheduleLocalSpacePtr);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
#endif
		// The space of the multiple affine function is obtained by artificially building the universal relation between two copies of the schedule set
		physicalScheduleFunctionPtr = isl_multi_aff_identity(isl_map_get_space(isl_map_from_domain_and_range(isl_set_universe(isl_space_copy(scheduleSpacePtr)), isl_set_universe(isl_space_copy(scheduleSpacePtr)))));
		
		if (physicalScheduleFunctionPtr == NULL)
			return isl_stat_error;
		
#ifdef MOREVERBOSE
		fprintf(stream, "Physical schedule to be built:\n");
		fflush(stream);
		printer = isl_printer_print_multi_aff(printer, physicalScheduleFunctionPtr);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
#endif
		
		// Parameter 1 of isl_aff_var_on_domain is annotated __isl_take
		physicalScheduleDimensionPtr = isl_aff_var_on_domain(scheduleLocalSpacePtr, isl_dim_set, parallelIteratorPos);
		
		if (physicalScheduleDimensionPtr == NULL)
			return isl_stat_error;
		
#ifdef VERBOSE
		fprintf(stream, "Physical schedule dimension to be built:\n");
		fflush(stream);
		printer = isl_printer_print_aff(printer, physicalScheduleDimensionPtr);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
#endif
		
		physicalScheduleDimensionPtr = isl_aff_set_coefficient_val(physicalScheduleDimensionPtr, isl_dim_in, parallelIteratorPos, isl_val_inv(isl_val_int_from_si(optionsHdl, N[i])));
		physicalScheduleDimensionPtr = isl_aff_floor(physicalScheduleDimensionPtr);
		
#ifdef MOREVERBOSE
		fprintf(stream, "Physical schedule dimension built:\n");
		fflush(stream);
		printer = isl_printer_print_aff(printer, physicalScheduleDimensionPtr);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
#endif
		
		 physicalScheduleFunctionPtr = isl_multi_aff_set_aff(physicalScheduleFunctionPtr, parallelIteratorPos, physicalScheduleDimensionPtr);
		 
#ifdef VERBOSE
		fprintf(stream, "Physical schedule built:\n");
		fflush(stream);
		printer = isl_printer_print_multi_aff(printer, physicalScheduleFunctionPtr);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return isl_stat_error;
		} 
		
		fprintf(stream, "\n");
#endif
		
		modifiedPolyhedralModelPtr[i] -> flattenedSchedule = isl_union_map_apply_range(schedulePtr, isl_union_map_from_map(isl_map_from_multi_aff(physicalScheduleFunctionPtr)));
		
#ifdef VERBOSE
		fprintf(stream, "Physical schedule as a relation:\n");
		fflush(stream);
		printer = isl_printer_print_union_map(printer, modifiedPolyhedralModelPtr[i] -> flattenedSchedule);
		
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

isl_union_set * polyhedral_slice_build (FILE * stream, isl_union_map * flattenedSchedulePtr, isl_union_map * linearizedSchedulePtr, isl_point * datePtr) {
#ifdef MOREVERBOSE
	// Pointer to the printer
	isl_printer * printer = NULL;
#endif
	// Pointer to the singleton set of the point
	isl_union_set * vectorSetPtr = NULL;
	
	vectorSetPtr = isl_union_map_domain(isl_union_map_intersect_range(linearizedSchedulePtr, isl_union_set_from_point(isl_point_copy(datePtr))));
	
	if(vectorSetPtr == NULL)
		return NULL;
	
#ifdef MOREVERBOSE
	printer = isl_printer_to_file(isl_union_set_get_ctx(vectorSetPtr), stream);
		
	if(printer == NULL) {
		error(stream, "Memory allocation problem :(");
		return NULL;
	} 
		
	isl_printer_set_indent(printer, moreIndent);
	
	fprintf(stream, "Schedule date(s): ");
	
	printer = isl_printer_print_union_set(printer, vectorSetPtr);
	
	if(printer == NULL) {
		error(stream, "Printing problem :(");
		return NULL;
	} 
	
	fprintf(stream, "\n");
	fflush(stream);	
	
	isl_printer_free(printer);
#endif
	
	return isl_union_map_domain(isl_union_map_intersect_range(flattenedSchedulePtr, vectorSetPtr));
}


isl_bool findOutermostParallel (__isl_keep isl_schedule_node * node, void * user) {
	// Depth of the current node
	int current = -1;
	// Pointer to the outermost parallel dimension found until now
	int * depthPtr = (int *)user;
	// Outermost parallel dimension found until now
	int depth = *(depthPtr);
	
	if (isl_schedule_node_get_type(node) == isl_schedule_node_band) {
		
		// We assume that the outermost parallel dimension is parallel for all the members
		if (isl_schedule_node_band_member_get_coincident(node, 0) == isl_bool_true) {
			current = isl_schedule_node_get_schedule_depth(node);
			
			if (depth < 0 || current < depth) {
				*(depthPtr) = current;
			}
			
			return STOP;
		} else {
			return GO_ON;
		}
		
	} else {
		return GO_ON;
	}
}
