#ifndef FRAME_ALLOCATOR_H
#define FRAME_ALLOCATOR_H

#include <stdint.h>
#include <pthread.h>
#include "paginacion.h" 
#include "tlb.h" // Necesitamos conocer la estructura del TLB

struct frame_allocator {
    int *frames;
    int total_frames;
    int free_frames;
    pthread_mutex_t lock;
};

void init_frame_allocator(int num_frames);
int64_t allocate_free_frame();
void free_frame_allocator();

// Recibe tlb y vpn para poder invalidar despues
void register_frame_allocation(struct page_table_entry *pte, int frame, struct tlb *tlb, uint64_t vpn);
int64_t evict_frame();
// Limpia punteros huérfanos si un hilo muere antes de tiempo
void remove_orphaned_pointers(struct page_table *pt, struct tlb *tlb);

#endif // FRAME_ALLOCATOR_H