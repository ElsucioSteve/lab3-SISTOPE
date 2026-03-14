#include <stdlib.h>
#include "workloads.h"
#include "segmentacion.h"


void generate_va_segmentation(int workload_type, struct segment_table *table, unsigned int *thread_seed, 
                              int *seg_id_out, uint64_t *offset_out) {
    
    int seg_id;
    uint64_t offset;
    int num_segs = table->num_segments;

    if (workload_type == WORKLOAD_UNIFORM) {
        //  Seleccion uniforme
        seg_id = rand_r(thread_seed) % num_segs;
        
        uint64_t limit = table->segments[seg_id].limit;
        
        // Generamos un offset que el 90% de las veces este dentro y el 10% fuera
        if ((rand_r(thread_seed) % 100) < 90) {
            offset = rand_r(thread_seed) % limit;
        } else {
            offset = limit + (rand_r(thread_seed) % 100); // Forzamos Segfault
        }
    } 
    else { // WORKLOAD_80_20
        int is_hot = (rand_r(thread_seed) % 100) < 80;
        int hot_count = num_segs * 0.2;
        if (hot_count == 0) hot_count = 1; // Al menos uno es "hot"

        if (is_hot) {
            seg_id = rand_r(thread_seed) % hot_count;
        } else {
            seg_id = hot_count + (rand_r(thread_seed) % (num_segs - hot_count));
        }
        offset = rand_r(thread_seed) % table->segments[seg_id].limit;
    }

    *seg_id_out = seg_id;
    *offset_out = offset;
}


void generate_va_paging(int workload_type, int num_pages, int page_size, unsigned int *seed, uint64_t *vpn, uint64_t *offset) {
    // El offset es siempre un valor aleatorio dentro del tamaño de la pagina
    *offset = rand_r(seed) % page_size;

    if (workload_type == WORKLOAD_80_20) {
        // El 80% de los accesos van al 20% de las paginas ("Hot Pages")
        int hot_pages = (num_pages * 20) / 100;
        if (hot_pages == 0) hot_pages = 1; // Minimo 1 pagina caliente
        
        int is_hot = (rand_r(seed) % 100) < 80;

        if (is_hot) {
            *vpn = rand_r(seed) % hot_pages;
        } else {
            *vpn = hot_pages + (rand_r(seed) % (num_pages - hot_pages));
        }
    } else {
        // Uniforme: Cualquier pagina con la misma probabilidad
        *vpn = rand_r(seed) % num_pages;
    }
}