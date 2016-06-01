/*
 * Definition of a type employed to represent the manipulated polyhedral model
 */
#ifndef MODEL_H
#define MODEL_H

#include <isl/union_set.h>
#include <isl/union_map.h>

typedef struct {
	unsigned parallelIteratorPos;
	isl_union_set * instanceSet;
	isl_union_map * flattenedSchedule;
	isl_union_map * allocation;
	isl_union_map * remappedMayReads;
	isl_union_map * remappedMayWrites;
	isl_union_map * remappedMustWrites;
	isl_union_map * linearizedSchedule;
} manipulated_polyhedral_model; 

typedef struct {
	unsigned ** access;
	unsigned n;
} dataset_type;

typedef struct {
	dataset_type ** datasetArray;
	unsigned numTypes;
	unsigned numTranslates;
	unsigned numProcessors;
} dataset_type_array;


manipulated_polyhedral_model ** manipulated_polyhedral_model_array_alloc(unsigned);
void manipulated_polyhedral_model_array_free(manipulated_polyhedral_model ** , unsigned);

dataset_type_array * dataset_type_array_alloc(unsigned, unsigned);
isl_stat dataset_type_array_add(dataset_type_array *, unsigned **);
void dataset_type_array_fprintf(FILE *, dataset_type_array *);
void dataset_type_array_free(dataset_type_array *);

#endif /* MODEL_H */
