/*
Jordan Gonzalez Riquelme, rut: 20.186.261-2
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>

#include "segmentacion.h"
#include "paginacion.h"
#include "workloads.h"

/* =========================================================================
 * Estructura para almacenar la configuracion del simulador
 * ========================================================================= */
typedef struct {
    char mode[10];         // "seg" o "page"
    int threads;           // Default: 1
    int ops_per_thread;    // Default: 1000
    char workload[20];     // "uniform" o "80-20"
    int seed;              // Default: 42
    int unsafe;            // Default: 0 (false)
    int stats;             // Default: 0 (false)
    
    // Configuracion especifica de Segmentacion
    int segments;          // Default: 4
    char seg_limits[256];  // Default: "4096"
    
    // Configuracion especifica de Paginacion
    int pages;             // Default: 64
    int frames;            // Default: 32
    int page_size;         // Default: 4096
    int tlb_size;          // Default: 16
    char tlb_policy[10];   // Default: "fifo"
    char evict_policy[10]; // Default: "fifo"
} SimulatorConfig;

// Instancia global con los valores por defecto
SimulatorConfig config = {
    .mode = "", .threads = 1, .ops_per_thread = 1000, .workload = "uniform",
    .seed = 42, .unsafe = 0, .stats = 0,
    .segments = 4, .seg_limits = "4096",
    .pages = 64, .frames = 32, .page_size = 4096, .tlb_size = 16,
    .tlb_policy = "fifo", .evict_policy = "fifo"
};

/* =========================================================================
 * Metricas Globales y Argumentos de Hilos
 * ========================================================================= */
// Instancias globales para cada modo (Los structs vienen de sus respectivos .h)
struct seg_metrics global_seg_metrics = {0, 0, 0, PTHREAD_MUTEX_INITIALIZER};
struct page_metrics global_page_metrics = {0, 0, 0, 0, 0, PTHREAD_MUTEX_INITIALIZER};

typedef struct {
    int thread_id;
    // Guardamos las metricas separadas para no enredarnos
    struct seg_metrics local_seg;
    struct page_metrics local_page;
} ThreadArgs;


/* =========================================================================
 * Funcion para parsear los argumentos de la terminal
 * ========================================================================= */
void parse_arguments(int argc, char *argv[]) {
    int c;
    while (1) {
        static struct option long_options[] = {
            {"mode",           required_argument, 0, 'm'},
            {"threads",        required_argument, 0, 't'},
            {"ops-per-thread", required_argument, 0, 'o'},
            {"workload",       required_argument, 0, 'w'},
            {"seed",           required_argument, 0, 's'},
            {"unsafe",         no_argument,       0, 'u'},
            {"stats",          no_argument,       0, 'x'},
            {"segments",       required_argument, 0, 'g'},
            {"seg-limits",     required_argument, 0, 'l'},
            {"pages",          required_argument, 0, 'p'},
            {"frames",         required_argument, 0, 'f'},
            {"page-size",      required_argument, 0, 'z'},
            {"tlb-size",       required_argument, 0, 'b'},
            {"tlb-policy",     required_argument, 0, 'y'},
            {"evict-policy",   required_argument, 0, 'e'},
            {0, 0, 0, 0} 
        };

        int option_index = 0;
        c = getopt_long(argc, argv, "m:t:o:w:s:uxg:l:p:f:z:b:y:e:", long_options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 'm': strncpy(config.mode, optarg, sizeof(config.mode)-1); break;
            case 't': config.threads = atoi(optarg); break;
            case 'o': config.ops_per_thread = atoi(optarg); break;
            case 'w': strncpy(config.workload, optarg, sizeof(config.workload)-1); break;
            case 's': config.seed = atoi(optarg); break;
            case 'u': config.unsafe = 1; break;
            case 'x': config.stats = 1; break;
            case 'g': config.segments = atoi(optarg); break;
            case 'l': strncpy(config.seg_limits, optarg, sizeof(config.seg_limits)-1); break;
            case 'p': config.pages = atoi(optarg); break;
            case 'f': config.frames = atoi(optarg); break;
            case 'z': config.page_size = atoi(optarg); break;
            case 'b': config.tlb_size = atoi(optarg); break;
            case 'y': strncpy(config.tlb_policy, optarg, sizeof(config.tlb_policy)-1); break;
            case 'e': strncpy(config.evict_policy, optarg, sizeof(config.evict_policy)-1); break;
            default: exit(EXIT_FAILURE);
        }
    }

    // =========================================================================
    // VALIDACION ESTRICTA DE ARGUMENTOS
    // =========================================================================

    // 1. Validar --mode (Obligatorio)
    if (strcmp(config.mode, "seg") != 0 && strcmp(config.mode, "page") != 0) {
        fprintf(stderr, "Error: El parametro --mode es obligatorio y debe ser 'seg' o 'page'.\n");
        exit(EXIT_FAILURE);
    }

    // 2. Validar --workload
    if (strcmp(config.workload, "uniform") != 0 && strcmp(config.workload, "80-20") != 0) {
        fprintf(stderr, "Error: El parametro --workload debe ser 'uniform' o '80-20'.\n");
        exit(EXIT_FAILURE);
    }

    // 3. Validar variables globales basicas
    if (config.threads <= 0 || config.ops_per_thread <= 0) {
        fprintf(stderr, "Error: Threads y ops_per_thread deben ser mayores a 0.\n");
        exit(EXIT_FAILURE);
    }

    // 4. Validaciones exclusivas por modo
    if (strcmp(config.mode, "seg") == 0) {
        if (config.segments <= 0) {
            fprintf(stderr, "Error: El numero de segmentos (--segments) debe ser mayor a 0.\n");
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(config.mode, "page") == 0) {
        if (config.pages <= 0) {
            fprintf(stderr, "Error: El numero de paginas (--pages) debe ser mayor a 0.\n");
            exit(EXIT_FAILURE);
        }
        if (config.frames <= 0) {
            fprintf(stderr, "Error: El numero de frames (--frames) debe ser mayor a 0.\n");
            exit(EXIT_FAILURE);
        }
        if (config.page_size <= 0) {
            fprintf(stderr, "Error: El tamaño de pagina (--page-size) debe ser mayor a 0.\n");
            exit(EXIT_FAILURE);
        }
        if (config.tlb_size < 0) { // Ojo: tlb_size puede ser 0
            fprintf(stderr, "Error: El tamaño del TLB (--tlb-size) no puede ser negativo.\n");
            exit(EXIT_FAILURE);
        }
        if (strcmp(config.tlb_policy, "fifo") != 0) {
            fprintf(stderr, "Error: La politica de TLB (--tlb-policy) solo soporta 'fifo'.\n");
            exit(EXIT_FAILURE);
        }
        if (strcmp(config.evict_policy, "fifo") != 0) {
            fprintf(stderr, "Error: La politica de reemplazo (--evict-policy) solo soporta 'fifo'.\n");
            exit(EXIT_FAILURE);
        }
    }
}

/* =========================================================================
 * Rutina de Hilos: SEGMENTACION
 * ========================================================================= */
void* thread_routine_seg(void* arg) {
    ThreadArgs *t_args = (ThreadArgs*) arg;
    unsigned int thread_seed = config.seed + t_args->thread_id;

    struct segment_table *my_table = init_segment_table(config.segments, config.seg_limits);
    if (!my_table) pthread_exit(NULL);

    int seg_id;
    uint64_t offset;
    struct timespec start, end;
    int workload_type = (strcmp(config.workload, "uniform") == 0) ? WORKLOAD_UNIFORM : WORKLOAD_80_20;

    for (int i = 0; i < config.ops_per_thread; i++) {
        generate_va_segmentation(workload_type, my_table, &thread_seed, &seg_id, &offset);

        clock_gettime(CLOCK_MONOTONIC, &start);
        int64_t pa = translate_segment_address(my_table, seg_id, offset);
        clock_gettime(CLOCK_MONOTONIC, &end);

        t_args->local_seg.total_time_ns += (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);

        if (pa == -1) {
            t_args->local_seg.segfaults++;
        } else {
            t_args->local_seg.translations_ok++;
        }
    }

    if (!config.unsafe) pthread_mutex_lock(&global_seg_metrics.lock);
    global_seg_metrics.translations_ok += t_args->local_seg.translations_ok;
    global_seg_metrics.segfaults += t_args->local_seg.segfaults;
    global_seg_metrics.total_time_ns += t_args->local_seg.total_time_ns;
    if (!config.unsafe) pthread_mutex_unlock(&global_seg_metrics.lock);

    free_segment_table(my_table);
    pthread_exit(NULL);
}

/* =========================================================================
 * Rutina de Hilos: PAGINACION
 * ========================================================================= */
void* thread_routine_page(void* arg) {
    ThreadArgs *t_args = (ThreadArgs*) arg;
    unsigned int thread_seed = config.seed + t_args->thread_id;

    struct page_table *my_pt = init_page_table(config.pages);
    struct tlb *my_tlb = init_tlb(config.tlb_size);
    if (!my_pt) pthread_exit(NULL);
    
    int workload_type = (strcmp(config.workload, "uniform") == 0) ? WORKLOAD_UNIFORM : WORKLOAD_80_20;
    struct timespec start, end;
    uint64_t vpn, offset;

    for (int i = 0; i < config.ops_per_thread; i++) {
        generate_va_paging(workload_type, config.pages, config.page_size, &thread_seed, &vpn, &offset);

        clock_gettime(CLOCK_MONOTONIC, &start);
        
		translate_page_address(my_pt, my_tlb, vpn, offset, config.page_size, &t_args->local_page);
        clock_gettime(CLOCK_MONOTONIC, &end);

        t_args->local_page.total_time_ns += (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
        
        
    }

    if (!config.unsafe) pthread_mutex_lock(&global_page_metrics.lock);
    
    global_page_metrics.tlb_hits += t_args->local_page.tlb_hits;
    global_page_metrics.tlb_misses += t_args->local_page.tlb_misses;
    global_page_metrics.page_faults += t_args->local_page.page_faults;
    global_page_metrics.total_time_ns += t_args->local_page.total_time_ns;
    global_page_metrics.evictions += t_args->local_page.evictions;
    if (!config.unsafe) pthread_mutex_unlock(&global_page_metrics.lock);

    free_page_table(my_pt);
    free_tlb(my_tlb);
    pthread_exit(NULL);
}


/* =========================================================================
 * MAIN
 * ========================================================================= */
int main(int argc, char *argv[]) {
    parse_arguments(argc, argv);

    // Si es paginacion, pedimos la RAM global antes de que nazcan los hilos
    if (strcmp(config.mode, "page") == 0) {
        init_frame_allocator(config.frames);
    }

    pthread_t threads[config.threads];
    ThreadArgs thread_args[config.threads];
    struct timespec sim_start, sim_end;
    
    clock_gettime(CLOCK_MONOTONIC, &sim_start);

    for (int i = 0; i < config.threads; i++) {
        thread_args[i].thread_id = i;
        memset(&thread_args[i].local_seg, 0, sizeof(struct seg_metrics));
        memset(&thread_args[i].local_page, 0, sizeof(struct page_metrics));
        
        void *routine = NULL;
        if (strcmp(config.mode, "seg") == 0) {
            routine = thread_routine_seg;
        } else if (strcmp(config.mode, "page") == 0) {
            routine = thread_routine_page;
        }
        
        if (pthread_create(&threads[i], NULL, routine, &thread_args[i]) != 0) {
            fprintf(stderr, "Error al crear el hilo %d\n", i);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < config.threads; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &sim_end);
    
    double total_time_sec = (sim_end.tv_sec - sim_start.tv_sec) + (sim_end.tv_nsec - sim_start.tv_nsec) / 1000000000.0;
    double total_ops = (double)config.threads * config.ops_per_thread;
    double throughput = total_ops / total_time_sec;

    // =========================================================================
    // ARTIFACT JSON Y SALIDA DE CONSOLA
    // =========================================================================
    FILE *json_file = fopen("out/summary.json", "w");
    if (json_file) {
        fprintf(json_file, "{\n");
        fprintf(json_file, "  \"mode\": \"%s\",\n", config.mode);
        
        if (strcmp(config.mode, "seg") == 0) {
            // --------- REPORTE SEGMENTACION ---------
            unsigned long avg_time_ns = (global_seg_metrics.translations_ok + global_seg_metrics.segfaults > 0) ? 
                                        global_seg_metrics.total_time_ns / (global_seg_metrics.translations_ok + global_seg_metrics.segfaults) : 0;
            
            // JSON
            fprintf(json_file, "  \"config\": {\n");
            fprintf(json_file, "    \"threads\": %d,\n", config.threads);
            fprintf(json_file, "    \"ops_per_thread\": %d,\n", config.ops_per_thread);
            fprintf(json_file, "    \"workload\": \"%s\",\n", config.workload);
            fprintf(json_file, "    \"seed\": %d,\n", config.seed);
            fprintf(json_file, "    \"unsafe\": %s,\n", config.unsafe ? "true" : "false");
            fprintf(json_file, "    \"segments\": %d\n", config.segments);
            fprintf(json_file, "  },\n");
            fprintf(json_file, "  \"metrics\": {\n");
            fprintf(json_file, "    \"translations_ok\": %lu,\n", global_seg_metrics.translations_ok);
            fprintf(json_file, "    \"segfaults\": %lu,\n", global_seg_metrics.segfaults);
            fprintf(json_file, "    \"avg_translation_time_ns\": %lu,\n", avg_time_ns);
            fprintf(json_file, "    \"throughput_ops_sec\": %.2f\n", throughput);
            fprintf(json_file, "  },\n");
            
            // Consola (Formato Enunciado)
            if (config.stats) {
                printf("========================================\n");
                printf("SIMULADOR DE MEMORIA VIRTUAL\n");
                printf("Modo: SEGMENTACION\n");
                printf("========================================\n");
                printf("Configuracion:\n");
                printf("Threads: %d\n", config.threads);
                printf("Ops por thread: %d\n", config.ops_per_thread);
                printf("Workload: %s\n", config.workload);
                printf("Seed: %d\n", config.seed);
                printf("Segmentos: %d\n", config.segments);
                printf("Metricas Globales:\n");
                printf("OK: %lu\n", global_seg_metrics.translations_ok);
                printf("Segfaults: %lu\n", global_seg_metrics.segfaults);
                printf("Avg Time: %lu ns\n", avg_time_ns);
                printf("Metricas por Thread:\n");
                for (int i = 0; i < config.threads; i++) {
                    printf("Thread %d: OK: %lu | Segfaults: %lu\n", 
                           i, thread_args[i].local_seg.translations_ok, thread_args[i].local_seg.segfaults);
                }
                printf("Tiempo total: %.2f segundos\n", total_time_sec);
                printf("Throughput: %.2f ops/seg\n", throughput);
                printf("========================================\n");
            }

        } else if (strcmp(config.mode, "page") == 0) {
            // --------- REPORTE PAGINACION ---------
            unsigned long total_lookups = global_page_metrics.tlb_hits + global_page_metrics.tlb_misses;
            double hit_rate = (total_lookups > 0) ? (double)global_page_metrics.tlb_hits / total_lookups : 0.0;
            unsigned long avg_time_ns = (total_ops > 0) ? global_page_metrics.total_time_ns / total_ops : 0;

            // JSON
            fprintf(json_file, "  \"config\": {\n");
            fprintf(json_file, "    \"threads\": %d,\n", config.threads);
            fprintf(json_file, "    \"ops_per_thread\": %d,\n", config.ops_per_thread);
            fprintf(json_file, "    \"workload\": \"%s\",\n", config.workload);
            fprintf(json_file, "    \"seed\": %d,\n", config.seed);
            fprintf(json_file, "    \"unsafe\": %s,\n", config.unsafe ? "true" : "false");
            fprintf(json_file, "    \"pages\": %d,\n", config.pages);
            fprintf(json_file, "    \"frames\": %d,\n", config.frames);
            fprintf(json_file, "    \"page_size\": %d,\n", config.page_size);
            fprintf(json_file, "    \"tlb_size\": %d,\n", config.tlb_size);
            fprintf(json_file, "    \"tlb_policy\": \"%s\",\n", config.tlb_policy);
            fprintf(json_file, "    \"evict_policy\": \"%s\"\n", config.evict_policy);
            fprintf(json_file, "  },\n");
            fprintf(json_file, "  \"metrics\": {\n");
            fprintf(json_file, "    \"tlb_hits\": %lu,\n", global_page_metrics.tlb_hits);
            fprintf(json_file, "    \"tlb_misses\": %lu,\n", global_page_metrics.tlb_misses);
            fprintf(json_file, "    \"hit_rate\": %.3f,\n", hit_rate);
            fprintf(json_file, "    \"page_faults\": %lu,\n", global_page_metrics.page_faults);
            fprintf(json_file, "    \"evictions\": %lu,\n", global_page_metrics.evictions);
            fprintf(json_file, "    \"avg_translation_time_ns\": %lu,\n", avg_time_ns);
            fprintf(json_file, "    \"throughput_ops_sec\": %.2f\n", throughput);
            fprintf(json_file, "  },\n");
            
            // Consola (Formato Enunciado)
            if (config.stats) {
                printf("========================================\n");
                printf("SIMULADOR DE MEMORIA VIRTUAL\n");
                printf("Modo: PAGINACION\n");
                printf("========================================\n");
                printf("Configuracion:\n");
                printf("Threads: %d\n", config.threads);
                printf("Ops por thread: %d\n", config.ops_per_thread);
                printf("Workload: %s\n", config.workload);
                printf("Seed: %d\n", config.seed);
                printf("Pages: %d | Frames: %d | Page Size: %d\n", config.pages, config.frames, config.page_size);
                printf("TLB Size: %d | TLB Policy: %s | Evict Policy: %s\n", config.tlb_size, config.tlb_policy, config.evict_policy);
                printf("Metricas Globales:\n");
                printf("TLB Hits: %lu | TLB Misses: %lu | Hit Rate: %.3f\n", global_page_metrics.tlb_hits, global_page_metrics.tlb_misses, hit_rate);
                printf("Page Faults: %lu | Evictions: %lu\n", global_page_metrics.page_faults, global_page_metrics.evictions);
                printf("Avg Time: %lu ns\n", avg_time_ns);
                printf("Metricas por Thread:\n");
                for (int i = 0; i < config.threads; i++) {
                    printf("Thread %d: TLB Hits: %lu | Misses: %lu | Faults: %lu | Evict: %lu\n", 
                           i, thread_args[i].local_page.tlb_hits, thread_args[i].local_page.tlb_misses, 
                           thread_args[i].local_page.page_faults, thread_args[i].local_page.evictions);
                }
                printf("Tiempo total: %.2f segundos\n", total_time_sec);
                printf("Throughput: %.2f ops/seg\n", throughput);
                printf("========================================\n");
            }
            
            free_frame_allocator(); // Liberamos RAM fisica al apagar
        }
        
        fprintf(json_file, "  \"runtime_sec\": %.6f\n", total_time_sec);
        fprintf(json_file, "}\n");
        fclose(json_file);
    }

    return EXIT_SUCCESS;
}