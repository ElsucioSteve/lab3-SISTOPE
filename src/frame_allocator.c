#include <stdlib.h>
#include <stdio.h>
#include "frame_allocator.h"
#include "tlb.h" // Necesitamos esto para llamar a tlb_invalidate

// Instancia global
struct frame_allocator global_allocator;

// =========================================================================
// COLA FIFO PARA EVICTION (Global)
// =========================================================================
struct fifo_entry {
    struct page_table_entry *pte; // Puntero a la pagina que ocupa el frame
    int frame;                    // Numero de frame
    struct tlb *tlb;              // Puntero al TLB del hilo dueño
    uint64_t vpn;                 // VPN asociado para invalidarlo
};

struct fifo_entry *eviction_queue = NULL;
int fifo_head = 0;
int fifo_tail = 0;
int fifo_count = 0;
// =========================================================================

void init_frame_allocator(int num_frames) {
    global_allocator.total_frames = num_frames;
    global_allocator.free_frames = num_frames;
    global_allocator.frames = (int *)calloc(num_frames, sizeof(int));
    pthread_mutex_init(&global_allocator.lock, NULL);

    // Inicializamos la cola circular FIFO
    eviction_queue = (struct fifo_entry *)malloc(num_frames * sizeof(struct fifo_entry));
    fifo_head = 0;
    fifo_tail = 0;
    fifo_count = 0;
}

int64_t allocate_free_frame() {
    pthread_mutex_lock(&global_allocator.lock);
    
    for (int i = 0; i < global_allocator.total_frames; i++) {
        if (global_allocator.frames[i] == 0) { // 0 = libre
            global_allocator.frames[i] = 1;    // Marcar como ocupado
            global_allocator.free_frames--;
            pthread_mutex_unlock(&global_allocator.lock);
            return (int64_t)i;
        }
    }

    pthread_mutex_unlock(&global_allocator.lock);
    return -1; // No hay memoria (Activa el Eviction)
}

// Registra una pagina recien cargada en la cola FIFO
void register_frame_allocation(struct page_table_entry *pte, int frame, struct tlb *tlb, uint64_t vpn) {
    pthread_mutex_lock(&global_allocator.lock);
    
    if (fifo_count < global_allocator.total_frames) {
        eviction_queue[fifo_tail].pte = pte;
        eviction_queue[fifo_tail].frame = frame;
        eviction_queue[fifo_tail].tlb = tlb; // Guardamos el TLB
        eviction_queue[fifo_tail].vpn = vpn; // Guardamos el VPN
        fifo_tail = (fifo_tail + 1) % global_allocator.total_frames;
        fifo_count++;
    }
    
    pthread_mutex_unlock(&global_allocator.lock);
}

// Expulsa la pagina mas antigua (FIFO) y devuelve su frame
int64_t evict_frame() {
    pthread_mutex_lock(&global_allocator.lock);
    
    if (fifo_count == 0) {
        pthread_mutex_unlock(&global_allocator.lock);
        return -1; // Fallo critico
    }

    // 1. Tomamos a la victima (la mas antigua)
    struct fifo_entry victim = eviction_queue[fifo_head];
    
    // 2. Le quitamos el frame a su tabla de paginas (Invalida)
    if (victim.pte != NULL) {
        victim.pte->valid = 0;
        victim.pte->frame_number = -1;
    }

    // 3. --- TLB SHOOTDOWN ---
    if (victim.tlb != NULL) {
        tlb_invalidate(victim.tlb, victim.vpn);
    }
    // ---------------------------------

    // 4. Avanzamos el head de la cola circular (sacamos a la victima)
    fifo_head = (fifo_head + 1) % global_allocator.total_frames;
    fifo_count--;

    pthread_mutex_unlock(&global_allocator.lock);
    
    // 5. Retornamos el frame que ahora quedo "libre"
    return victim.frame;
}

void free_frame_allocator() {
    if (global_allocator.frames) free(global_allocator.frames);
    if (eviction_queue) free(eviction_queue);
    pthread_mutex_destroy(&global_allocator.lock);
}

void remove_orphaned_pointers(struct page_table *pt, struct tlb *tlb) {
    if (!eviction_queue) return; // Si no hay paginacion activa (ej: segmentacion)

    pthread_mutex_lock(&global_allocator.lock);
    
    for (int i = 0; i < global_allocator.total_frames; i++) {
        // Limpiar TLB huérfano
        if (tlb != NULL && eviction_queue[i].tlb == tlb) {
            eviction_queue[i].tlb = NULL;
        }
        
        // Limpiar PTE huérfano (revisamos si el puntero cae dentro del array de la tabla)
        if (pt != NULL && eviction_queue[i].pte >= pt->entries && 
            eviction_queue[i].pte < (pt->entries + pt->num_pages)) {
            eviction_queue[i].pte = NULL;
        }
    }
    
    pthread_mutex_unlock(&global_allocator.lock);
}