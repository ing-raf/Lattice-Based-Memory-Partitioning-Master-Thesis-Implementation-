/*
 * Definitions of building functions of the lattice - based memory partitioning
 * technique
 */

#ifndef PARTITIONING_H_
#define PARTITIONING_H_

#include<pet.h>
#include<isl/ctx.h>

#include "model.h"

isl_stat parse_architecture (FILE *, char *, architecture_type *, unsigned *, unsigned *, unsigned **, unsigned ***);
isl_stat parse_processors_allocation_UMA (FILE *, char *, unsigned *, unsigned, unsigned **);
isl_stat parse_processors_allocation_NUMA (FILE *, char *, unsigned *, unsigned, unsigned **, unsigned **, unsigned **); 
isl_stat parse_input(FILE *, isl_ctx *, char**,  pet_scop **, unsigned);
isl_stat virtual_allocation (FILE *, isl_ctx *, pet_scop **, manipulated_polyhedral_model **, unsigned, unsigned *);
isl_set *** parse_lattices (FILE *, isl_ctx *, unsigned *, unsigned, unsigned); 
isl_stat physical_schedule (FILE *, isl_ctx *, pet_scop **, manipulated_polyhedral_model **, unsigned, unsigned *);
isl_stat allocation_constraint (FILE *, isl_ctx *, manipulated_polyhedral_model **, unsigned, unsigned *);
isl_stat eliminate_parameters (FILE *, char **, char **, pet_scop **, manipulated_polyhedral_model **, unsigned);
isl_stat parse_parameters (char **, char **, unsigned, parameters ***);
isl_stat linearize_dates (FILE *, manipulated_polyhedral_model **, unsigned);
isl_union_set * polyhedral_slice_build (FILE *, isl_union_map *, isl_union_map *, isl_point *);
// isl_union_set * instant_local_slice_build (FILE *, isl_union_map *, isl_union_map *, isl_point *, unsigned);
isl_union_set * instant_local_slice_build (FILE * , isl_union_set *, isl_union_map *, isl_union_map *, isl_union_map *, isl_point *, unsigned);
isl_set * concurrent_dataset_build (FILE *, manipulated_polyhedral_model **, isl_union_set **, unsigned);
isl_set ** instant_local_dataset_build (FILE *, manipulated_polyhedral_model **, isl_union_set **, unsigned *, unsigned);
isl_stat evaluate_fundamental_lattice (FILE *, isl_set *, isl_set **, unsigned, unsigned *);
isl_stat access_matrix_fundamental_lattice (FILE *, isl_set **, isl_set **, unsigned, unsigned, unsigned **);
isl_stat MILPsolve(FILE *, unsigned, dataset_type_array **, unsigned, unsigned, unsigned *, unsigned **); 

#endif /* PARTITIONING_H_ */
