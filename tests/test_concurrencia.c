#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

// Simulamos una metrica compartida
struct test_metrics {
    unsigned long total_sum;
    pthread_mutex_t lock;
};

struct test_metrics shared_data = {0, PTHREAD_MUTEX_INITIALIZER};
int ops_per_thread = 10000;

void* add_to_metric(void* arg) {
    (void)arg; // Evitar warning de variable sin uso
    for (int i = 0; i < ops_per_thread; i++) {
        pthread_mutex_lock(&shared_data.lock);
        shared_data.total_sum++;
        pthread_mutex_unlock(&shared_data.lock);
    }
    return NULL;
}

int main() {
    printf("Iniciando tests de Concurrencia (Mutex)...\n");

    int num_threads = 10;
    pthread_t threads[num_threads];

    // Lanzar hilos
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, add_to_metric, NULL);
    }

    // Esperar hilos
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Verificar suma exacta
    unsigned long expected = num_threads * ops_per_thread;
    assert(shared_data.total_sum == expected);

    printf("Test de Concurrencia superado (Condicion de carrera prevenida)!\n");
    return 0;
}