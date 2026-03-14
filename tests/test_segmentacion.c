#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "segmentacion.h"

int main() {
    printf("Iniciando tests de Segmentacion...\n");

    // Crear 2 segmentos con limites 1000 y 2000
    struct segment_table *st = init_segment_table(2, "1000,2000");
    assert(st != NULL);
    assert(st->num_segments == 2);
    
    // Verificar que las bases y limites se asignaron bien
    assert(st->segments[0].base == 0);
    assert(st->segments[0].limit == 1000);
    assert(st->segments[1].base == 1000); // Empieza donde termina el anterior
    assert(st->segments[1].limit == 2000);

    // Prueba 1: Acceso valido en Segmento 0
    int64_t pa1 = translate_segment_address(st, 0, 500);
    assert(pa1 == 500); // 0 + 500

    // Prueba 2: Acceso valido en Segmento 1
    int64_t pa2 = translate_segment_address(st, 1, 500);
    assert(pa2 == 1500); // 1000 + 500

    // Prueba 3: SEGFAULT (offset mayor al limite)
    int64_t pa3 = translate_segment_address(st, 0, 1500);
    assert(pa3 == -1);

    free_segment_table(st);
    printf("Todos los tests de Segmentacion pasaron con exito!\n");
    return 0;
}