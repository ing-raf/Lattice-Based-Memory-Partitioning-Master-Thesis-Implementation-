#ifdef MOREVERBOSE
#define VERBOSE
#endif

#include<stdlib.h>

#include<isl/union_set.h>

#include "support.h"
#include "partitioning.h"

isl_stat linearize_date(isl_point *, void *);

isl_stat linearize_dates(manipulated_polyhedral_model ** modifiedPolyhedralModel, unsigned numTasks) {
	// Pointer to the schedule applied to the instance set of the current task
	isl_union_set * appliedSchedule = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	
	for(int i = 0; i < numTasks; i++) {
		
		appliedSchedule = isl_union_set_apply(modifiedPolyhedralModel[i] -> instanceSet, modifiedPolyhedralModel[i] -> flattenedSchedule);
		
		if (appliedSchedule == NULL)
			return isl_stat_error;
		
#ifdef VERBOSE
		printf("Applied schedule:\n");
		fflush(stdout);
		isl_union_set_dump(appliedSchedule);
#endif
		
		outcome = isl_union_set_foreach_point(appliedSchedule, linearize_date, NULL);
		
		if (outcome == isl_stat_error)
			return isl_stat_error;
	}
	
	return isl_stat_ok;
	
}

isl_stat linearize_date (isl_point * vector, void * bho) {
	
#ifdef MOREVERBOSE
	printf("Point being linearized:");
	fflush(stdout);
	isl_point_dump(vector);
#endif
	
	return isl_stat_ok;
}