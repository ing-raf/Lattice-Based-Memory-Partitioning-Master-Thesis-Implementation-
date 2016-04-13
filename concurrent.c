#ifdef MOREVERBOSE
#define VERBOSE
#endif

#include<stdlib.h>

#include<isl/union_set.h>

#include "support.h"
#include "partitioning.h"

typedef struct {
	isl_union_set * appliedSchedule;
	isl_union_map * partialLinearization;
} linearize_date_params;

typedef struct {
	unsigned count;
} set_cardinality_params;

isl_stat linearize_date(isl_point *, void *);
isl_stat set_cardinality(isl_point *, void *);

isl_stat linearize_dates(manipulated_polyhedral_model ** modifiedPolyhedralModel, unsigned numTasks) {
	// Parameters for the callback function
	linearize_date_params * linearizationParams = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	
	for(int i = 0; i < numTasks; i++) {
		
		linearizationParams = malloc(sizeof(linearize_date_params));
		
		if (linearizationParams == NULL)
			return isl_stat_error;
		
		linearizationParams -> appliedSchedule = isl_union_set_apply(modifiedPolyhedralModel[i] -> instanceSet, modifiedPolyhedralModel[i] -> flattenedSchedule);
		
		if (linearizationParams -> appliedSchedule == NULL)
			return isl_stat_error;
		
#ifdef VERBOSE
		printf("Applied schedule:\n");
		fflush(stdout);
		isl_union_set_dump(linearizationParams -> appliedSchedule);
#endif
		
		outcome = isl_union_set_foreach_point(linearizationParams -> appliedSchedule, linearize_date, (void *)linearizationParams);
		
		if (outcome == isl_stat_error)
			return isl_stat_error;
		
		// Be clean
		free(linearizationParams);
	}
	
	return isl_stat_ok;
	
}

isl_stat linearize_date (isl_point * vector, void * user) {
	// Pointer to the input parameters
	linearize_date_params * params = (linearize_date_params *)user;
	// Pointer to the singleton set of the point
	isl_union_set * singletonPtr = NULL;
	// Pointer to the set of lexicographically smaller dates
	isl_union_set * lexLtSetPtr = NULL;
	// Parameters for the callback function
	set_cardinality_params * cardParams = NULL;
	// Result of a subroutine
	isl_stat outcome = isl_stat_ok;
	
	
#ifdef MOREVERBOSE
	printf("Point being linearized:");
	fflush(stdout);
	isl_point_dump(vector);
#endif
	
	singletonPtr = isl_union_set_from_point(vector);
	
	if (singletonPtr == NULL)
		return isl_stat_error;
	
#ifdef MOREVERBOSE
	printf("Singleton set:");
	fflush(stdout);
	isl_union_set_dump(singletonPtr);
	printf("Applied schedule:");
	fflush(stdout);
	isl_union_set_dump(params -> appliedSchedule);
#endif
	
	lexLtSetPtr = isl_union_map_domain(isl_union_set_lex_lt_union_set(isl_union_set_copy(params -> appliedSchedule), singletonPtr));
	
	if (lexLtSetPtr == NULL)
		return isl_stat_error;
	
#ifdef MOREVERBOSE
	printf("Set of lexicographically smaller dates:");
	fflush(stdout);
	isl_union_set_dump(lexLtSetPtr);
#endif
	
	cardParams = malloc(sizeof(set_cardinality_params));
	
	if (cardParams == NULL)
		return isl_stat_error;
	
	cardParams -> count = 0;
	
	outcome = isl_union_set_foreach_point (lexLtSetPtr, set_cardinality, (void *)cardParams);
	
	if (outcome == isl_stat_error)
		return isl_stat_error;
	
#ifdef MOREVERBOSE
	printf("Cardinality of the set: %d\n", cardParams -> count);
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