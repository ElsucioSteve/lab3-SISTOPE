
#ifndef SEGMENTACION_H
#define SEGMENTACION_H

#include <stdint.h>
#include <pthread.h>

// Representa una entrada en la tabla de segmentos
struct segment_entry {
    uint64_t base;  // Direccion fısica base (absoluta en memoria)
    uint64_t limit; // Lımite del segmento en bytes (maximo offset valido)
};

// Representa la tabla de segmentos privada de cada thread
struct segment_table {
    struct segment_entry *segments; // Arreglo de entradas
    int num_segments;               // Cantidad de segmentos en la tabla
};


// Estructura para almacenar las metricas, aplicable globalmente y por thread
struct seg_metrics {
    unsigned long translations_ok;
    unsigned long segfaults;
    unsigned long total_time_ns; // Para calcular avg_translation_time_ns
    
    // Mutex para evitar condiciones de carrera en metricas globales (Modo SAFE)
    pthread_mutex_t lock; 
};


struct segment_table* init_segment_table(int num_segments, const char* seg_limits_csv);


int64_t translate_segment_address(struct segment_table *table, int seg_id, uint64_t offset);


void free_segment_table(struct segment_table *table);

#endif // SEGMENTACION_H