/*
 * Definitions of building functions of the lattice - based memory partitioning
 * technique
 */

#ifndef PARTITIONING_H_
#define PARTITIONING_H_

#include<pet.h>
#include<isl/ctx.h>

#include "model.h"

isl_stat parse_input(FILE *, isl_ctx *, char**,  pet_scop **, unsigned);
isl_stat virtual_allocation (FILE *, isl_ctx *, pet_scop **, manipulated_polyhedral_model **, unsigned);
isl_stat physical_schedule (FILE *, isl_ctx *, pet_scop **, manipulated_polyhedral_model **, unsigned);
isl_stat eliminate_parameters (FILE *, pet_scop **, manipulated_polyhedral_model **, unsigned);
isl_stat linearize_dates(FILE *, manipulated_polyhedral_model **, unsigned);

#endif /* PARTITIONING_H_ */
