CC = gcc
CFLAGS = -g -Wall -I.
LIBS = -lgd -pthread
DIR ?= Datasets/Dataset-B
#DIR ?= Datasets/Dataset-A

all:process-photos-parallel-B

# Compilação da Parte B 
process-photos-parallel-B: process-photos-parallel-B.c image-lib.c image-lib.h
	$(CC) $(CFLAGS) process-photos-parallel-B.c image-lib.c -o process-photos-parallel-B $(LIBS)

clean:
	rm -f  process-photos-parallel-B

clean-all: clean
	rm -fr ./Result-image-dir
	rm -rf $(DIR)/Result-image-dir
	rm -f $(DIR)/*.txt
	rm -f process-photos-parallel-B