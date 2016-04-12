/*
 * Definitions of building functions of the lattice - based memory partitioning
 * technique
 */

#ifndef PARTITIONING_H_
#define PARTITIONING_H_

#include<pet.h>
#include<isl/ctx.h>

typedef struct {
	isl_union_map * flattenedSchedule;
	isl_union_map * remappedMayReads;
	isl_union_map * remappedMayWrites;
	isl_union_map * remappedMustWrites;
} manipulated_polyhedral_model;

isl_stat parse_input(isl_ctx *, char**,  pet_scop **, unsigned);
isl_stat virtual_allocation (isl_ctx *, pet_scop **, manipulated_polyhedral_model *, unsigned);
isl_stat physical_schedule (isl_ctx *, pet_scop **, manipulated_polyhedral_model *, unsigned);
isl_stat eliminate_parameters (manipulated_polyhedral_model *, unsigned);
void linearize_dates(isl_union_map**, unsigned);

#endif /* PARTITIONING_H_ */
