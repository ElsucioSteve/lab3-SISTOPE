#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "paginacion.h"
#include "frame_allocator.h" 

int main() {
    printf("Iniciando tests de Paginacion...\n");

    // Inicializar RAM con solo 2 frames (marcos)
    init_frame_allocator(2);
    struct page_table *pt = init_page_table(4); // 4 paginas virtuales
    struct tlb *tlb = init_tlb(2);              // TLB de 2 entradas
    struct page_metrics metrics = {0};
    int page_size = 4096;

    // Prueba 1: Page Fault (La pagina 0 no esta en RAM)
    int64_t pa1 = translate_page_address(pt, tlb, 0, 10, page_size, &metrics);
    assert(pa1 == 10); // Asignado al Frame 0
    assert(metrics.page_faults == 1);
    assert(metrics.tlb_misses == 1);

    // Prueba 2: TLB Hit (La pagina 0 ya esta en caché)
    int64_t pa2 = translate_page_address(pt, tlb, 0, 20, page_size, &metrics);
    assert(pa2 == 20); // Frame 0
    assert(metrics.tlb_hits == 1);

    // Prueba 3: Llenar la RAM (Frame 1 ocupado por Pagina 1)
    translate_page_address(pt, tlb, 1, 0, page_size, &metrics);
    assert(metrics.page_faults == 2);

    // Prueba 4: EVICTION (Se acabo la RAM, pedimos Pagina 2, debe botar a la Pagina 0)
    translate_page_address(pt, tlb, 2, 0, page_size, &metrics);
    assert(metrics.evictions == 1);
    assert(pt->entries[0].valid == 0); // La pagina 0 debe haber sido invalidada

    free_page_table(pt);
    free_tlb(tlb);
    free_frame_allocator();
    
    printf("Todos los tests de Paginacion pasaron con exito!\n");
    return 0;
}