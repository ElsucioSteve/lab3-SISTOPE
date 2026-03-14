CC = gcc
CFLAGS = -std=c11 -pthread -Wall -Wextra -Iinclude
SRC_DIR = src
OUT_DIR = out

# Encuentra todos los .c en src/ y define los .o correspondientes
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OUT_DIR)/%.o, $(SRCS))
TARGET = $(OUT_DIR)/simulator

# =========================================================================
# 1. COMPILACION (make o make all)
# =========================================================================
all: $(OUT_DIR) $(TARGET)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OUT_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# =========================================================================
# 2. EJECUCION POR DEFECTO (make run)
# =========================================================================
run: all
	@echo "\n================================================================="
	@echo " EJECUTANDO EJEMPLO POR DEFECTO (Paginacion Basica)"
	@echo "================================================================="
	./$(TARGET) --mode page --threads 2 --ops-per-thread 1000 --pages 64 --frames 32 --page-size 4096 --tlb-size 16 --stats

# =========================================================================
# 3. REPRODUCCION DE EXPERIMENTOS (make reproduce)
# =========================================================================
reproduce: all
	@echo "\n================================================================="
	@echo "EXPERIMENTO 1: Segmentacion (Estres de Concurrencia y Segfaults)"
	@echo "================================================================="
	./$(TARGET) --mode seg --threads 4 --ops-per-thread 25000 --segments 8 --seg-limits "1024,2048,4096,8192,1024,2048,4096,8192" --stats
	@cp out/summary.json out/summary_exp1.json

	@echo "\n================================================================="
	@echo "EXPERIMENTO 2: Paginacion (TLB Eficiente con Workload 80-20)"
	@echo "================================================================="
	./$(TARGET) --mode page --threads 2 --ops-per-thread 15000 --pages 128 --frames 64 --page-size 4096 --tlb-size 32 --workload 80-20 --stats
	@cp out/summary.json out/summary_exp2.json

	@echo "\n================================================================="
	@echo "EXPERIMENTO 3: Paginacion (Cuello de botella - Estres de Eviction)"
	@echo "================================================================="
	./$(TARGET) --mode page --threads 4 --ops-per-thread 5000 --pages 64 --frames 8 --page-size 4096 --tlb-size 4 --workload uniform --stats
	@cp out/summary.json out/summary_exp3.json

# =========================================================================
# 4. LIMPIEZA (make clean)
# =========================================================================
clean:
	rm -rf $(OUT_DIR)/*
	
# =========================================================================
# 5. TESTS UNITARIOS (make test)
# =========================================================================
test: all
	@echo "\n--- Compilando y ejecutando Tests ---"
	$(CC) $(CFLAGS) tests/test_segmentacion.c $(OUT_DIR)/segmentacion.o -o $(OUT_DIR)/test_seg
	$(CC) $(CFLAGS) tests/test_paginacion.c $(OUT_DIR)/paginacion.o $(OUT_DIR)/tlb.o $(OUT_DIR)/frame_allocator.o -o $(OUT_DIR)/test_page
	$(CC) $(CFLAGS) tests/test_concurrencia.c -o $(OUT_DIR)/test_conc
	
	@echo "\n[1/3] Ejecutando test_segmentacion:"
	@./$(OUT_DIR)/test_seg
	@echo "\n[2/3] Ejecutando test_paginacion:"
	@./$(OUT_DIR)/test_page
	@echo "\n[3/3] Ejecutando test_concurrencia:"
	@./$(OUT_DIR)/test_conc