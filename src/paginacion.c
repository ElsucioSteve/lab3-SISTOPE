#include <stdlib.h>
#include "paginacion.h"
#include "frame_allocator.h"
#include "tlb.h"
#include <time.h> // Necesario para nanosleep

struct page_table* init_page_table(int num_pages) {
    struct page_table *pt = (struct page_table *)malloc(sizeof(struct page_table));
    if (!pt) return NULL;

    pt->num_pages = num_pages;
    pt->entries = (struct page_table_entry *)malloc(num_pages * sizeof(struct page_table_entry));
    
    for (int i = 0; i < num_pages; i++) {
        pt->entries[i].frame_number = INVALID_FRAME; 
        pt->entries[i].valid = 0;
    }
    return pt;
}

int64_t translate_page_address(struct page_table *pt, struct tlb *tlb, uint64_t vpn, uint64_t offset, uint64_t page_size, struct page_metrics *metrics) {
    // 1. Intentar en TLB
    int64_t frame = tlb_lookup(tlb, vpn);
    
    if (frame != -1) {
        metrics->tlb_hits++; // TLB HIT!
        return (frame * page_size) + offset;
    }

    metrics->tlb_misses++; // TLB MISS

    // 2. Buscar en Tabla de Paginas
    if (vpn >= (uint64_t)pt->num_pages) return -1; // Direccion invalida

    if (pt->entries[vpn].valid) {
        frame = pt->entries[vpn].frame_number;
    } else {
        // 3. PAGE FAULT: No esta en RAM
        metrics->page_faults++; 
        
        // --- PUNTO 6: Simular delay de disco ---
        struct timespec disk_delay = {0, 1000000L}; // 1 milisegundo (1,000,000 ns)
        nanosleep(&disk_delay, NULL);
        // ---------------------------------------

        // Pedir frame libre
        frame = allocate_free_frame();
        
        // Si no hay frames libres, ¡EVICTION!
        if (frame == -1) {
            frame = evict_frame();
            if (frame == -1) return -1; // Error fatal de memoria
            metrics->evictions++;       // Sumamos a la metrica
        }
        
        pt->entries[vpn].frame_number = frame;
        pt->entries[vpn].valid = 1;

        // Registramos la nueva pagina en la cola FIFO del Allocator
        // Pasamos tlb y vpn para el posible "TLB Shootdown" en el futuro
        register_frame_allocation(&pt->entries[vpn], frame, tlb, vpn);
    }

    // 4. Actualizar TLB
    tlb_insert(tlb, vpn, frame);

    return (frame * page_size) + offset;
}

void free_page_table(struct page_table *pt) {
    if (pt) {
        // Limpiamos la basura antes de destruir la tabla
        remove_orphaned_pointers(pt, NULL); 
        
        if (pt->entries) free(pt->entries);
        free(pt);
    }
}