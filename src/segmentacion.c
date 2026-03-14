#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "segmentacion.h"

struct segment_table* init_segment_table(int num_segments, const char* seg_limits_csv) {
    // 1. Pedir memoria para la tabla
    struct segment_table *table = malloc(sizeof(struct segment_table));
    if (!table) return NULL;

    table->num_segments = num_segments;
    table->segments = malloc(num_segments * sizeof(struct segment_entry));
    if (!table->segments) {
        free(table);
        return NULL;
    }

	// 2. Leer los limites separados por comas
    char *csv_copy = malloc(strlen(seg_limits_csv) + 1);
    strcpy(csv_copy, seg_limits_csv);
    char *token = strtok(csv_copy, ",");
    
    uint64_t current_base = 0;   // Empezamos en la direccion fisica 0
    uint64_t last_limit = 4096;  // Valor por defecto

    for (int i = 0; i < num_segments; i++) {
        if (token != NULL) {
            last_limit = strtoull(token, NULL, 10);
            token = strtok(NULL, ",");
        }
        
        // Asignamos base y limite
        table->segments[i].base = current_base;
        table->segments[i].limit = last_limit;
        
        // El proximo segmento empezara donde termina este
        current_base += last_limit; 
    }

    free(csv_copy);
    return table;
}

int64_t translate_segment_address(struct segment_table *table, int seg_id, uint64_t offset) {
    // Verificar si el segmento existe
    if (seg_id < 0 || seg_id >= table->num_segments) {
        return -1; // Segfault
    }

    // Verificar si el offset se sale del limite 
    if (offset >= table->segments[seg_id].limit) {
        return -1; // Segfault
    }

    // Traduccion exitosa
    return (int64_t)(table->segments[seg_id].base + offset);
}

void free_segment_table(struct segment_table *table) {
    if (table) {
        if (table->segments) free(table->segments);
        free(table);
    }
}