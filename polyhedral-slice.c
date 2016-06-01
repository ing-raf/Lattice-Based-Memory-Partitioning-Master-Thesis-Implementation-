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

typedef struct {
	isl_union_set * extendedVectorSet;
	unsigned p;
} extend_date_params;

isl_bool findOutermostParallel (isl_schedule_node *, void *);
isl_stat extend_date (isl_point *, void *);

isl_stat physical_schedule (FILE * stream, isl_ctx * optionsHdl, pet_scop ** polyhedralModelPtr, manipulated_polyhedral_model ** modifiedPolyhedralModelPtr, unsigned numTasks, unsigned * n) {
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

		if (parallelIteratorPos >= 0)
			modifiedPolyhedralModelPtr[i] -> parallelIteratorPos = parallelIteratorPos;
		else {
			warning(stream, "No parallel dimension found");
			return isl_stat_error;
		}
		
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
		
		physicalScheduleDimensionPtr = isl_aff_set_coefficient_val(physicalScheduleDimensionPtr, isl_dim_in, parallelIteratorPos, isl_val_inv(isl_val_int_from_si(optionsHdl, n[i])));
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

isl_stat allocation_constraint (FILE * stream, isl_ctx * optionsHdl, manipulated_polyhedral_model ** modifiedPolyhedralModelPtr, unsigned numTasks, unsigned * n) {
	// Dimensionality of the domain of the schedule of the current task
	unsigned scheduleDim = 0;
	#ifdef VERBOSE
		// Pointer to the printer
		isl_printer * printer = NULL;
	#endif
	// Pointer to the space of the domain of the schedule of the current task
	isl_space * scheduleSpacePtr = NULL;
	// Pointer to the local space of the domain of the schedule of the current task
	isl_local_space * scheduleLocalSpacePtr = NULL;
	// Pointer to the space of the range of the space - time mapping of the current task
	isl_space * spaceTimeMappingSpacePtr = NULL;
	// Pointer to the physical schedule as a multiple function
	isl_multi_aff * spaceTimeMappingFunctionPtr = NULL;
	// Pointer to the space - time mapping dimension currently build
	isl_aff * spaceTimeDimensionPtr = NULL;

	for (int i = 0; i < numTasks; i++) {
	
		// scheduleSpacePtr = isl_set_get_space(isl_set_from_union_set(isl_union_map_range(isl_union_map_copy(modifiedPolyhedralModelPtr[i] -> flattenedSchedule))));

		scheduleSpacePtr = isl_set_get_space(isl_set_from_union_set(isl_union_map_domain(isl_union_map_copy(modifiedPolyhedralModelPtr[i] -> flattenedSchedule))));
		
		if (scheduleSpacePtr == NULL)
			return isl_stat_error;	
		
		scheduleLocalSpacePtr = isl_local_space_from_space(isl_space_copy(scheduleSpacePtr));
		
		if (scheduleLocalSpacePtr == NULL)
			return isl_stat_error;
		
		scheduleDim = isl_local_space_dim(scheduleLocalSpacePtr, isl_dim_set);
		
		#ifdef VERBOSE
			info(stream, "Task %d)", i);
		
			printer = isl_printer_to_file(optionsHdl, stream);
			
			if(printer == NULL) {
				error(stream, "Memory allocation problem :(");
				return isl_stat_error;
			} 
			
			isl_printer_set_indent(printer, moreIndent);
			fprintf(stream, "Dimensionality of the schedule domain: %d\n", scheduleDim);
			fflush(stream);
			printer = isl_printer_print_local_space(printer, scheduleLocalSpacePtr);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\n");
		#endif

		// Parameters will be brought in by the schedule dimensions mapping. Employ here a number of parameters other than 0 would imply the necessity to properly name the parameters
		// spaceTimeMappingSpacePtr = isl_space_alloc(optionsHdl, 0, scheduleDim, scheduleDim + 1);
		
		// spaceTimeMappingFunctionPtr = isl_multi_aff_zero(spaceTimeMappingSpacePtr);
		
		// if (spaceTimeMappingFunctionPtr == NULL)
		// 	return isl_stat_error;
		
		// #ifdef MOREVERBOSE
		// 	fprintf(stream, "Space - time mapping to be built:\n");
		// 	fflush(stream);
		// 	printer = isl_printer_print_multi_aff(printer, spaceTimeMappingFunctionPtr);
			
		// 	if(printer == NULL) {
		// 		error(stream, "Printing problem :(");
		// 		return isl_stat_error;
		// 	} 
			
		// 	fprintf(stream, "\n");
		// #endif
		
		// Copying the schedule dimensions on the space - time mapping
		// for (unsigned i = 0; i < scheduleDim; i++) {
		// 	// isl_aff_var_on_domain requires a set space
		// 	spaceTimeDimensionPtr = isl_aff_var_on_domain(isl_local_space_copy(scheduleLocalSpacePtr), isl_dim_set, i);
		
		// 	if (spaceTimeDimensionPtr == NULL)
		// 		return isl_stat_error;
			
		// 	spaceTimeMappingFunctionPtr = isl_multi_aff_set_aff(spaceTimeMappingFunctionPtr, i, spaceTimeDimensionPtr);

		// 	if (spaceTimeMappingFunctionPtr == NULL) 
		// 		return isl_stat_error;
		// }

		// #ifdef VERBOSE
		// 	fprintf(stream, "Time part of the space - time mapping:\n");
		// 	fflush(stream);
		// 	printer = isl_printer_print_multi_aff(printer, spaceTimeMappingFunctionPtr);
			
		// 	if(printer == NULL) {
		// 		error(stream, "Printing problem :(");
		// 		return isl_stat_error;
		// 	} 
			
		// 	fprintf(stream, "\n");
		// #endif
		
		// Building the allocation constraint
		spaceTimeDimensionPtr = isl_aff_var_on_domain(isl_local_space_copy(scheduleLocalSpacePtr), isl_dim_set, modifiedPolyhedralModelPtr[i] -> parallelIteratorPos);
		
		if (spaceTimeDimensionPtr == NULL)
			return isl_stat_error;
		
		spaceTimeDimensionPtr = isl_aff_mod_val(spaceTimeDimensionPtr, isl_val_int_from_si(optionsHdl, n[i]));

		if (spaceTimeDimensionPtr == NULL)
			return isl_stat_error;
		
		#ifdef MOREVERBOSE
			fprintf(stream, "Allocation constraint built:\n");
			fflush(stream);
			printer = isl_printer_print_aff(printer, spaceTimeDimensionPtr);
			
			if(printer == NULL) {
				error(stream, "Printing problem :(");
				return isl_stat_error;
			} 
			
			fprintf(stream, "\n");
		#endif
		
		// spaceTimeMappingFunctionPtr = isl_multi_aff_set_aff(spaceTimeMappingFunctionPtr, scheduleDim, spaceTimeDimensionPtr);

		// if (spaceTimeMappingFunctionPtr == NULL) 
		// 	return isl_stat_error;
		 
		// #ifdef VERBOSE
		// 	fprintf(stream, "Space - time mapping built:\n");
		// 	fflush(stream);
		// 	printer = isl_printer_print_multi_aff(printer, spaceTimeMappingFunctionPtr);
			
		// 	if(printer == NULL) {
		// 		error(stream, "Printing problem :(");
		// 		return isl_stat_error;
		// 	} 
			
		// 	fprintf(stream, "\n");
		// #endif

		
		// modifiedPolyhedralModelPtr[i] -> spaceTimeMapping = isl_union_map_apply_range(isl_union_map_copy(modifiedPolyhedralModelPtr[i] -> flattenedSchedule), isl_union_map_from_map(isl_map_from_multi_aff(spaceTimeMappingFunctionPtr)));

		// if (modifiedPolyhedralModelPtr[i] -> spaceTimeMapping == NULL)
		// 	return isl_stat_error;

		modifiedPolyhedralModelPtr[i] -> allocation = isl_union_map_from_map(isl_map_from_aff(spaceTimeDimensionPtr));

		// if (modifiedPolyhedralModelPtr[i] -> spaceTimeMapping == NULL)
		// 	return isl_stat_error;
		
		#ifdef VERBOSE
			fprintf(stream, "Space - time mapping as a relation:\n");
			fflush(stream);
			// printer = isl_printer_print_union_map(printer, modifiedPolyhedralModelPtr[i] -> spaceTimeMapping);
			printer = isl_printer_print_union_map(printer, modifiedPolyhedralModelPtr[i] -> allocation);
			
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

// isl_union_set * instant_local_slice_build (FILE * stream, isl_union_map * linearizedSchedulePtr, isl_union_map * spaceTimeMappingPtr, isl_point * datePtr, unsigned processor) {
// 	#ifdef MOREVERBOSE
// 		// Pointer to the printer
// 		isl_printer * printer = NULL;
// 	#endif
// 	// Pointer to the *singleton) set of the point
// 	isl_union_set * vectorSetPtr = NULL;
// 	// Pointer to the (singleton) set of the equivalent space - time point
// 	isl_union_set * extendedVectorSetPtr = NULL;
// 	// Parameters for the callback function
// 	extend_date_params * extendDateParams = NULL;
// 	// Result of a subroutine
// 	isl_stat outcome = isl_stat_error;
	
// 	vectorSetPtr = isl_union_map_domain(isl_union_map_intersect_range(linearizedSchedulePtr, isl_union_set_from_point(isl_point_copy(datePtr))));
	
// 	if(vectorSetPtr == NULL)
// 		return NULL;
	
// 	#ifdef MOREVERBOSE
// 		printer = isl_printer_to_file(isl_union_set_get_ctx(vectorSetPtr), stream);
			
// 		if(printer == NULL) {
// 			error(stream, "Memory allocation problem :(");
// 			return NULL;
// 		} 
			
// 		isl_printer_set_indent(printer, moreIndent);
		
// 		fprintf(stream, "Schedule date(s): ");
		
// 		printer = isl_printer_print_union_set(printer, vectorSetPtr);
		
// 		if(printer == NULL) {
// 			error(stream, "Printing problem :(");
// 			return NULL;
// 		} 
		
// 		fprintf(stream, "\n");
// 		fflush(stream);	
		
// 	#endif

// 	extendDateParams = (extend_date_params *)malloc(sizeof(extend_date_params));

// 	extendDateParams -> p = processor;
// 	extendDateParams -> extendedVectorSet = NULL;

// 	// Now we extend each date in the set of dates corresponding to the current linearized date with the space dimensions
// 	outcome = isl_union_set_foreach_point(vectorSetPtr, extend_date, extendDateParams);

// 	if (outcome == isl_stat_error) {
// 		error(stream, "Problems during the dates extension");
// 		return NULL;
// 	}

// 	extendedVectorSetPtr = isl_union_set_coalesce(extendDateParams -> extendedVectorSet);

// 	// Be clean
// 	free(extendDateParams);

// 	#ifdef MOREVERBOSE
// 		fprintf(stream, "Extended schedule date(s): ");
		
// 		printer = isl_printer_print_union_set(printer, extendedVectorSetPtr);

// 		fprintf(stream, "\nPhysical space time schedule: ");
		
// 		printer = isl_printer_print_union_map(printer, spaceTimeMappingPtr);
		
// 		if(printer == NULL) {
// 			error(stream, "Printing problem :(");
// 			return NULL;
// 		} 
		
// 		fprintf(stream, "\n");
// 		fflush(stream);	
		
// 		isl_printer_free(printer);
// 	#endif

// 	// isl_union_map_dump(isl_union_map_reverse(spaceTimeMappingPtr));
// 	// isl_union_map_dump(isl_union_map_intersect_domain(isl_union_map_reverse(spaceTimeMappingPtr), extendedVectorSetPtr));
// 	// isl_union_set_dump(isl_union_map_range(isl_union_map_intersect_domain(isl_union_map_reverse(spaceTimeMappingPtr), extendedVectorSetPtr)));

// 	// isl_union_set_dump(isl_union_set_apply(extendedVectorSetPtr, isl_union_map_reverse(isl_union_map_copy(spaceTimeMappingPtr))));

// 	// return isl_union_map_range(isl_union_map_intersect_domain(isl_union_map_reverse(isl_union_map_copy(spaceTimeMappingPtr)), extendedVectorSetPtr));

// 	return isl_union_map_domain(isl_union_map_intersect_range(isl_union_map_copy(spaceTimeMappingPtr), extendedVectorSetPtr));
// }

// new version
isl_union_set * instant_local_slice_build (FILE * stream, isl_union_set * instanceSetPtr, isl_union_map * flattenedSchedulePtr, isl_union_map * linearizedSchedulePtr, isl_union_map * allocationPtr, isl_point * datePtr, unsigned processor) {
	#ifdef MOREVERBOSE
		// Pointer to the printer
		isl_printer * printer = NULL;
	#endif
	// Pointer to a space for the point representing the processor
	isl_space * processorSpacePtr = NULL;
	// Pointer to a point representing the processor
	isl_point * processorPointPtr = NULL;
	// Pointer to the (singleton) set representing the processor
	isl_union_set * processorSetPtr = NULL;
	// Pointer to the polyhedral slice
	isl_union_set * polyhedralSlicePtr = NULL;
	// Pointer to the local slice
	isl_union_set * localSlicePtr = NULL;

	processorSpacePtr = isl_space_set_alloc(isl_union_map_get_ctx(flattenedSchedulePtr), 0, 1);

	if(processorSpacePtr == NULL)
		return NULL;

	// Parameter 1 of isl_point_zero is annotated __isl_take
	processorPointPtr = isl_point_zero(processorSpacePtr);

	if(processorPointPtr == NULL)
		return NULL;

	processorPointPtr = isl_point_add_ui(processorPointPtr, isl_dim_set, 0, processor);

	if(processorPointPtr == NULL)
		return NULL;

	// Base processor accounting has been translated to the caller
	// To account for the base processor
	// processorPointPtr = isl_point_sub_ui(processorPointPtr, isl_dim_set, 0, OFFSET[TASK[processor]]);

	// Parameter 1 of iisl_union_set_from_point is annotated __isl_take
	processorSetPtr = isl_union_set_from_point(processorPointPtr);

	if(processorSetPtr == NULL)
		return NULL;

	#ifdef MOREVERBOSE
		printer = isl_printer_to_file(isl_union_map_get_ctx(flattenedSchedulePtr), stream);
			
		if(printer == NULL) {
			error(stream, "Memory allocation problem :(");
			return NULL;
		} 
			
		isl_printer_set_indent(printer, moreIndent);
		
		fprintf(stream, "Singleton set of the processor: ");
		
		printer = isl_printer_print_union_set(printer, processorSetPtr);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return NULL;
		} 
		
		fprintf(stream, "\n");
		fflush(stream);	
		
	#endif

	polyhedralSlicePtr = polyhedral_slice_build(stream, isl_union_map_copy(flattenedSchedulePtr), isl_union_map_copy(linearizedSchedulePtr), datePtr);

	if(polyhedralSlicePtr == NULL)
		return NULL;

	localSlicePtr = isl_union_set_intersect(isl_union_map_domain(isl_union_map_intersect_range(isl_union_map_copy(allocationPtr), processorSetPtr)), isl_union_set_copy(instanceSetPtr));
	
	if(localSlicePtr == NULL)
		return NULL;

	#ifdef MOREVERBOSE
		fprintf(stream, "Local slice: ");
		
		printer = isl_printer_print_union_set(printer, localSlicePtr);
		
		if(printer == NULL) {
			error(stream, "Printing problem :(");
			return NULL;
		} 
		
		fprintf(stream, "\n");
		fflush(stream);	
		
		isl_printer_free(printer);
	#endif

	return isl_union_set_intersect(polyhedralSlicePtr, localSlicePtr);;
}


isl_bool findOutermostParallel (isl_schedule_node * node, void * user) {
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

isl_stat extend_date (isl_point * schedulePointPtr, void * user){
	// Number of dimensions of the input point
	unsigned scheduleDim = 0;
	// Pointer to the input parameters
	extend_date_params * params = (extend_date_params *)user;
	// Space of the input point
	isl_space * pointSpacePtr = NULL;
	// Space of the output point
	isl_space * extendedPointSpacePtr = NULL;
	// Output point
	isl_point * extendedPointPtr = NULL;

	// WARNING: isl_point_get_space could return an union set space, that does not have set dimensions
	// SPORCATA: if this is the case, build a singleton isl_set of the point
	pointSpacePtr = isl_point_get_space(schedulePointPtr);

	if (pointSpacePtr == NULL)
		return isl_stat_error;

	scheduleDim = isl_space_dim(pointSpacePtr, isl_dim_set);

	if (scheduleDim == 0)
		return isl_stat_error;

	extendedPointSpacePtr = isl_space_set_alloc(isl_space_get_ctx(pointSpacePtr), isl_space_dim(pointSpacePtr, isl_dim_param), scheduleDim + 1);

	if (extendedPointSpacePtr == NULL)
		return isl_stat_error;

	extendedPointPtr = isl_point_zero(extendedPointSpacePtr);

	if (extendedPointPtr == NULL)
		return isl_stat_error;

	// Copying the schedule dimensions
	for (unsigned i = 0; i < scheduleDim; i++) {
		extendedPointPtr = isl_point_set_coordinate_val(extendedPointPtr, isl_dim_set, i, isl_point_get_coordinate_val(schedulePointPtr, isl_dim_set, i));

		if (extendedPointPtr == NULL)
			return isl_stat_error;
	}

	// Here we exploit the fact that the point has been allocated with all 0s
	extendedPointPtr = isl_point_add_ui(extendedPointPtr, isl_dim_set, scheduleDim, params -> p);

	// Base processor accounting has been translated to the caller
	// To account for the base processor
	// extendedPointPtr = isl_point_sub_ui(extendedPointPtr, isl_dim_set, scheduleDim, OFFSET[TASK[params -> p]]);

	if (extendedPointPtr == NULL)
		return isl_stat_error;

	if (params -> extendedVectorSet == NULL)
		params -> extendedVectorSet = isl_union_set_from_point(extendedPointPtr);
	else
		params -> extendedVectorSet = isl_union_set_union(params -> extendedVectorSet, isl_union_set_from_point(extendedPointPtr));

	if (params -> extendedVectorSet == NULL)
		return isl_stat_error;

	return isl_stat_ok;
}
