/*
 * Definitions of building functions of the lattice - based memory partitioning
 * technique
 */

#ifndef PARTITIONING_H_
#define PARTITIONING_H_

#include<pet.h>
#include<isl/ctx.h>

isl_stat virtual_allocation (isl_ctx*, pet_scop **, unsigned);

#endif /* PARTITIONING_H_ */
