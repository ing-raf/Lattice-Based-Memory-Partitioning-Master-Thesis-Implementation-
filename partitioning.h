/*
 * Definitions of building functions of the lattice - based memory partitioning
 * technique
 */

#ifndef PARTITIONING_H_
#define PARTITIONING_H_

#include<pet.h>
#include<isl/ctx.h>

typedef struct {
	isl_union_map * remappedMayReads;
	isl_union_map * remappedMayWrites;
	isl_union_map * remappedMustWrites;
} remapped_access_relations;

isl_stat virtual_allocation (isl_ctx*, pet_scop **, remapped_access_relations *, unsigned);

#endif /* PARTITIONING_H_ */
