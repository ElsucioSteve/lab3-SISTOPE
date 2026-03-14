#ifndef WORKLOADS_H
#define WORKLOADS_H

#include <stdint.h>
#include "segmentacion.h"

#define WORKLOAD_UNIFORM 0
#define WORKLOAD_80_20 1

void generate_va_segmentation(int workload_type, struct segment_table *table, unsigned int *thread_seed, 
                              int *seg_id_out, uint64_t *offset_out);
							  
void generate_va_paging(int workload_type, int num_pages, int page_size, unsigned int *seed, uint64_t *vpn, uint64_t *offset);

#endif

