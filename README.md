
Jordan Gonzalez Riquelme, rut: 20.186.261-2

# Simulador de Memoria Virtual

Este proyecto es un simulador de memoria virtual multihilo desarrollado en C (estándar C11). Permite evaluar y comparar el rendimiento de dos esquemas clásicos de administración de memoria: **Segmentación** y **Paginación**, midiendo métricas clave como latencia, throughput, tasas de acierto en TLB (Hit Rate) y reemplazos de página (Evictions).

---

## Requisitos
- Compilador `gcc` compatible con el estándar C11.
- Sistema operativo Linux/Unix (o Windows Subsystem for Linux - WSL).
- Librería `pthread` (incluida en distribuciones estándar de C).
- Herramienta `make` instalada.

---

## Compilación y Limpieza

El proyecto incluye un `Makefile` para automatizar todo el proceso. Abre tu terminal en la raíz del proyecto y utiliza los siguientes comandos:

- **Compilar el proyecto:**
  make
  # o también: make all

El ejecutable se generará en la carpeta out/ bajo el nombre simulator.

## Limpiar los archivos compilados (.o y ejecutables):

En la terminar de la raíz del proyecto utiliza el siguientes comando:
    make clean
    
## Ejecución Rápida (Ejemplo por defecto):

Si solo quieres ver el simulador funcionando rápidamente con una configuración de prueba (Paginación básica), ejecuta:
    make run

Esto compilará el código (si no lo está) y ejecutará un caso con 2 hilos, 1000 operaciones por hilo, 64 páginas y 32 frames.

## Reproducción de Experimentos (Evaluación):

Para ejecutar automáticamente los 3 experimentos requeridos en el enunciado y guardar sus resultados, utiliza:
    make reproduce
    
Este comando ejecutará de forma secuencial:

    Experimento 1: Segmentacion con Segfaults Controlados (estrés de Segfaults).
        Demostrar que la segmentacion detecta correctamente violaciones de lımite.

    Experimento 2: Impacto del TLB (Alta tasa de aciertos TLB con Workload 80-20).
        Comparar performance con y sin TLB usando el mismo workload y seed.

    Experimento 3: Thrashing con Multiples Threads (Alta tasa de Evictions por RAM limitada).
        Observar thrashing cuando hay mas paginas activas que frames disponibles.

Archivos Generados:
Al usar make reproduce, el sistema hará una copia de seguridad automática de las métricas en formato JSON dentro de la carpeta out/ y un reporte en consola:

    out/summary_exp1.json

    out/summary_exp2.json

    out/summary_exp3.json
    
## Uso Manual Avanzado:

Puedes ejecutar el simulador directamente desde la terminal con configuraciones personalizadas. A continuación se detallan los tipos de datos esperados y los rangos válidos para cada parámetro.

### Parámetros Globales:
- `--mode <string>`: Modo de simulación. **Obligatorio**.
  - **Valores válidos:** `"seg"` (Segmentación) o `"page"` (Paginación).
- `--threads <entero>`: Número de hilos concurrentes.
  - **Valores válidos:** Mayor a 0 (`N > 0`). (Defecto: 1).
- `--ops-per-thread <entero>`: Operaciones a realizar por cada hilo.
  - **Valores válidos:** Mayor a 0 (`N > 0`). (Defecto: 1000).
- `--workload <string>`: Distribución de probabilidad de las direcciones generadas.
  - **Valores válidos:** `"uniform"` o `"80-20"`. (Defecto: uniform).
- `--seed <entero>`: Semilla para la generación de números aleatorios.
  - **Valores válidos:** Cualquier número entero (`N >= 0`). (Defecto: 42).
- `--stats`: Flag (sin argumentos). Si se incluye, imprime el reporte detallado en la consola.
- `--unsafe`: Flag (sin argumentos). Si se incluye, desactiva los mutex globales para simular condiciones de carrera.

### Específicos de Segmentación (`--mode seg`):
- `--segments <entero>`: Cantidad de segmentos.
  - **Valores válidos:** Mayor a 0 (`N > 0`).
- `--seg-limits <string>`: Límites de cada segmento en bytes.
  - **Formato esperado:** Números enteros positivos separados por comas (Ej: `"1024,2048,4096"`).

### Específicos de Paginación (`--mode page`):
- `--pages <entero>`: Número de páginas virtuales disponibles.
  - **Valores válidos:** Mayor a 0 (`N > 0`).
- `--frames <entero>`: Número de marcos (frames) en la memoria física RAM.
  - **Valores válidos:** Mayor a 0 (`N > 0`).
- `--page-size <entero>`: Tamaño de cada página en bytes.
  - **Valores válidos:** Mayor a 0 (`N > 0`). (Usualmente potencias de 2, ej: 4096).
- `--tlb-size <entero>`: Cantidad de entradas en el caché TLB.
  - **Valores válidos:** Mayor o igual a 0 (`N >= 0`). Un valor de `0` desactiva el TLB.
- `--tlb-policy <string>`: Política de reemplazo del TLB.
  - **Valores válidos:** `"fifo"` (Única política soportada).
- `--evict-policy <string>`: Política de reemplazo de páginas en RAM.
  - **Valores válidos:** `"fifo"` (Única política soportada).

Ejemplo de ejecución manual:
    ./out/simulator --mode page --threads 4 --ops-per-thread 5000 --pages 128 --frames 16 --page-size 4096 --tlb-size 8 --stats
    
#### Ejemplos de Segmentación (`--mode seg`)

   Ejecuta 2 hilos con 4 segmentos usando los límites por defecto. 
   ./out/simulator --mode seg --threads 2 --ops-per-thread 1000 --segments 4 --stats
   
   Segmentación con Límites Personalizados (Forzando Segfaults):
   ./out/simulator --mode seg --threads 4 --ops-per-thread 5000 --segments 3 --seg-limits "1024,512,2048" --workload uniform --stats
   
   Prueba de Condiciones de Carrera (Modo Unsafe):
   ./out/simulator --mode seg --threads 8 --ops-per-thread 50000 --segments 4 --unsafe --stats
   
### Ejemplos de Paginación (--mode page)

    Paginación sin Caché (TLB Apagado):
    ./out/simulator --mode page --threads 2 --ops-per-thread 1000 --pages 64 --frames 64 --page-size 4096 --tlb-size 0 --stats
    
    Paginación Optimizada (Caché Efectivo):
    ./out/simulator --mode page --threads 4 --ops-per-thread 10000 --pages 64 --frames 32 --page-size 4096 --tlb-size 16 --workload 80-20 --stats
    
    Estresar el Algoritmo de Reemplazo (Eviction FIFO):
    ./out/simulator --mode page --threads 1 --ops-per-thread 5000 --pages 128 --frames 4 --page-size 4096 --tlb-size 2 --stats
    

## Pruebas Unitarias (Tests)

El proyecto incluye un conjunto de pruebas unitarias basadas en `assert` para garantizar el correcto funcionamiento de los módulos principales de forma aislada. Estas pruebas verifican la lógica de segmentación, paginación y la sincronización entre hilos.

Para compilar y ejecutar todas las pruebas automáticamente, utiliza el siguiente comando:
    make test
    
Este comando ejecutará tres suites de pruebas:

    test_segmentacion: Valida la creación de la tabla de segmentos, el cálculo correcto de las direcciones físicas y la correcta detección de accesos fuera de límite (Segfaults).

    test_paginacion: Valida la inicialización de la RAM (Frame Allocator), la resolución de direcciones (Page Faults), el funcionamiento de la memoria caché (TLB Hits y Misses) y la política de reemplazo FIFO (Evictions) cuando la RAM se llena.

    test_concurrencia: Ejecuta múltiples hilos compitiendo por modificar una métrica global. Valida que el uso de candados (pthread_mutex_t) evita correctamente las condiciones de carrera (Race Conditions), asegurando que no se pierdan datos.
    
## Salida (Artifacts):
    
Además del reporte en consola (si pasas el flag --stats), cada ejecución manual genera o sobreescribe un archivo summary.json en el directorio out/ con un resumen estructurado y parseable de las métricas de la última simulación.




