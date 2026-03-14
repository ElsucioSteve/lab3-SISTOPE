#ifndef TLB_H
#define TLB_H

#include <stdint.h>

struct tlb_entry {
    uint64_t vpn;
    uint64_t frame_number;
    int valid;
};

struct tlb {
    struct tlb_entry *entries;
    int size;
    int next_index; // Para la politica FIFO
    int count;      // Cuantas entradas estan ocupadas
};

struct tlb* init_tlb(int size);
int64_t tlb_lookup(struct tlb *tlb, uint64_t vpn);
void tlb_insert(struct tlb *tlb, uint64_t vpn, uint64_t frame);

// Para el TLB Shootdown 
void tlb_invalidate(struct tlb *tlb, uint64_t vpn);

void free_tlb(struct tlb *t);

#endif // TLB_H