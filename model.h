/*
 * Definition of a type employed to represent the manipulated polyhedral model
 */
#ifndef MODEL_H
#define MODEL_H

#include<isl/union_set.h>
#include<isl/union_map.h>

typedef struct {
	isl_union_set * instanceSet;
	isl_union_map * flattenedSchedule;
	isl_union_map * remappedMayReads;
	isl_union_map * remappedMayWrites;
	isl_union_map * remappedMustWrites;
} manipulated_polyhedral_model; 

manipulated_polyhedral_model ** manipulated_polyhedral_model_array_alloc(unsigned);
void manipulated_polyhedral_model_array_free(manipulated_polyhedral_model ** , unsigned);

#endif /* MODEL_H */
