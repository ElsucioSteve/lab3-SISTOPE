#include <stdlib.h>
#include "tlb.h"
#include "frame_allocator.h" // Necesario para limpiar punteros

struct tlb* init_tlb(int size) {
    struct tlb *t = (struct tlb *)malloc(sizeof(struct tlb));
    t->size = size;
    t->next_index = 0;
    t->count = 0;
    
    if (size > 0) {
        t->entries = (struct tlb_entry *)calloc(size, sizeof(struct tlb_entry));
    } else {
        t->entries = NULL;
    }
    return t;
}

int64_t tlb_lookup(struct tlb *tlb, uint64_t vpn) {
    if (!tlb || tlb->size == 0) return -1;

    for (int i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].vpn == vpn) {
            return (int64_t)tlb->entries[i].frame_number; // TLB HIT
        }
    }
    return -1; // TLB MISS
}

void tlb_insert(struct tlb *tlb, uint64_t vpn, uint64_t frame) {
    if (!tlb || tlb->size == 0) return;

    tlb->entries[tlb->next_index].vpn = vpn;
    tlb->entries[tlb->next_index].frame_number = frame;
    tlb->entries[tlb->next_index].valid = 1;

    // Movimiento circular FIFO
    tlb->next_index = (tlb->next_index + 1) % tlb->size;
    if (tlb->count < tlb->size) tlb->count++;
}

// Invalida una pagina en el TLB
void tlb_invalidate(struct tlb *tlb, uint64_t vpn) {
    if (!tlb || tlb->size == 0) return;
    
    for (int i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].vpn == vpn) {
            tlb->entries[i].valid = 0; 
            break; // Ya lo encontramos e invalidamos
        }
    }
}

void free_tlb(struct tlb *t) {
    if (t) {
        // Limpiamos la basura antes de destruir el TLB
        remove_orphaned_pointers(NULL, t);
        
        if (t->entries) free(t->entries);
        free(t);
    }
}