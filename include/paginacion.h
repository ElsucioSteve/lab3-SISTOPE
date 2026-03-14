#ifndef PAGINACION_H
#define PAGINACION_H

#include <stdint.h>
#include <pthread.h>
#include "tlb.h" // Incluimos la TLB para poder usarla en la traduccion

#define INVALID_FRAME ((uint64_t)-1)

struct page_table_entry {
    uint64_t frame_number;
    int valid;
};

struct page_table {
    struct page_table_entry *entries;
    int num_pages;
};

/* =========================================================================
 * Metricas de Paginacion
 * ========================================================================= */
struct page_metrics {
    unsigned long tlb_hits;
    unsigned long tlb_misses;
    unsigned long page_faults;
    unsigned long evictions;       //  Numero de paginas expulsadas de RAM
    unsigned long total_time_ns;   // Para calcular el promedio al final
    
    pthread_mutex_t lock;
};

struct page_table* init_page_table(int num_pages);
void free_page_table(struct page_table *pt);
int64_t translate_page_address(struct page_table *pt, struct tlb *tlb, uint64_t vpn, uint64_t offset, uint64_t page_size, struct page_metrics *metrics);

// Inicializa la memoria fisica (RAM)
void init_frame_allocator(int num_frames);

// Libera la memoria fisica
void free_frame_allocator();

#endif // PAGINACION_H