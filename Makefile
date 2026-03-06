CC = gcc
CFLAGS = -Wall -Wextra -pthread -Iinclude
SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=out/%.o)
TARGET = out/simulator

.PHONY: all run reproduce clean

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p out
	$(CC) $(CFLAGS) -o $@ $^

out/%.o: src/%.c
	@mkdir -p out
	$(CC) $(CFLAGS) -c $< -o $@

# Target para ejecución rápida
run: all
	./$(TARGET)

# Target para generar el reporte final solicitado
reproduce: all
	@echo "Ejecutando experimentos..."
	./$(TARGET) --mode seg --threads 4 > out/summary.json
	@echo "Resumen generado en out/summary.json"

clean:
	rm -rf out/*.o $(TARGET) out/summary.json