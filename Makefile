CC = gcc
CFLAGS = -g -Wall -I.
LIBS = -lgd -pthread
THREADS ?= 4
#DIR ?= Input-image-dir
DIR ?= Input-image-big


all: process-photos-serial process-photos-parallel-A process-photos-parallel-B


process-photos-serial: process-photos-serial.c image-lib.c image-lib.h
	$(CC) $(CFLAGS) process-photos-serial.c image-lib.c -o process-photos-serial -lgd

# Compilação da Parte A
process-photos-parallel-A: process-photos-parallel-A.c image-lib.c image-lib.h
	$(CC) $(CFLAGS) process-photos-parallel-A.c image-lib.c -o process-photos-parallel-A $(LIBS)

# Compilação da Parte B 
process-photos-parallel-B: process-photos-parallel-B.c image-lib.c image-lib.h
	$(CC) $(CFLAGS) process-photos-parallel-B.c image-lib.c -o process-photos-parallel-B $(LIBS)


clean:
	rm -f process-photos-serial process-photos-parallel-A process-photos-parallel-B


clean-all: clean
	rm -rf ./Result-image-dir
	rm -rf $(DIR)/Result-image-dir
	rm -f $(DIR)/*.txt



runB-size: process-photos-parallel-B
	@echo "A executar Parte B com $(THREADS) threads (ordem: size)..."
	./process-photos-parallel-B $(THREADS) -size


runB-name: process-photos-parallel-B
	@echo "A executar Parte B com $(THREADS) threads (ordem: name)..."
	./process-photos-parallel-B $(THREADS) -name