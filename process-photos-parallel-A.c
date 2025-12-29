#define _POSIX_C_SOURCE 199309L //para monotonic_clock
#include <gd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "image-lib.h"

#define RESULT_IMAGE_DIR "Result-image-dir"

/******************************************************************************
 * Instituto Superior Técnico
 * Programação Concorrente 2025/2026
 * Projeto - Parte A
 *
 * Grupo: 122
 * Aluno: Luis Miguel (100010)
 *
 * Ficheiro: process-photos-parallel-A.c
 * Descrição: Versão paralela do processamento de imagens usando Pthreads.
 *****************************************************************************/

typedef struct {
    char name[256];
    off_t size;
} ImageInfo;

typedef struct {
    int thread_id;
    int start_index;
    int end_index;
    ImageInfo *images;
    char *input_dir;
    struct timespec duration;
} ThreadArgs;

struct timespec start_total, end_total;
struct timespec start_seq, end_seq;

// Funções auxiliares para o qsort
/******************************************************************************
 * compare_name()
 *
 * Arguments: a - pointer to the first ImageInfo element (const void *)
 * b - pointer to the second ImageInfo element (const void *)
 * Returns: integer less than, equal to, or greater than zero depending on 
 * lexicographical comparison
 * Side-Effects: none
 *
 * Description: Comparator function used by qsort() to sort the image array 
 * alphabetically by the filename.
 *****************************************************************************/

int compare_name(const void *a, const void *b) {
    ImageInfo *imgA = (ImageInfo *)a;
    ImageInfo *imgB = (ImageInfo *)b;
    return strcmp(imgA->name, imgB->name);
}

/******************************************************************************
 * compare_size()
 *
 * Arguments: a - pointer to the first ImageInfo element (const void *)
 * b - pointer to the second ImageInfo element (const void *)
 * Returns: -1 if a is smaller than b, 1 if a is larger, 0 if equal
 * Side-Effects: none
 *
 * Description: Comparator function used by qsort() to sort the image array 
 * based on the file size (in bytes).
 *****************************************************************************/

 int compare_size(const void *a, const void *b) {
    ImageInfo *imgA = (ImageInfo *)a;
    ImageInfo *imgB = (ImageInfo *)b;
    if (imgA->size < imgB->size) return -1;
    if (imgA->size > imgB->size) return 1;
    return 0;
}

// Função de trabalho da thread
/******************************************************************************
 * worker_thread()
 *
 * Arguments: arg - pointer to ThreadArgs structure containing thread config
 * Returns: NULL (via pthread_exit)
 * Side-Effects: Reads JPEG files from the input directory, creates multiple
 * processed versions (contrast, blur, sepia, thumb, gray), 
 * writes them to the output directory, and updates the 
 * duration field in the arguments structure.
 *
 * Description: Main routine executed by each worker thread. It processes a 
 * specific range of images assigned by the main thread.
 *****************************************************************************/

void *worker_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    struct timespec start_thread, end_thread;
    
    clock_gettime(CLOCK_MONOTONIC, &start_thread);

    char in_path[2048];
    char out_path[2048];
    char res_dir_path[2048];
    
    sprintf(res_dir_path, "%s/%s/", args->input_dir, RESULT_IMAGE_DIR);

    for (int i = args->start_index; i < args->end_index; i++) {
        sprintf(in_path, "%s/%s", args->input_dir, args->images[i].name);
        
        gdImagePtr in_img = read_jpeg_file(in_path);
        if (!in_img) continue;

        gdImagePtr out_img;

        // Aplica as 5 transformações sequencialmente
        out_img = contrast_image(in_img);
        sprintf(out_path, "%scontrast_%s", res_dir_path, args->images[i].name);
        write_jpeg_file(out_img, out_path);
        gdImageDestroy(out_img);

        out_img = blur_image(in_img);
        sprintf(out_path, "%sblur_%s", res_dir_path, args->images[i].name);
        write_jpeg_file(out_img, out_path);
        gdImageDestroy(out_img);

        out_img = sepia_image(in_img);
        sprintf(out_path, "%ssepia_%s", res_dir_path, args->images[i].name);
        write_jpeg_file(out_img, out_path);
        gdImageDestroy(out_img);

        out_img = thumb_image(in_img);
        sprintf(out_path, "%sthumb_%s", res_dir_path, args->images[i].name);
        write_jpeg_file(out_img, out_path);
        gdImageDestroy(out_img);

        out_img = gray_image(in_img);
        sprintf(out_path, "%sgray_%s", res_dir_path, args->images[i].name);
        write_jpeg_file(out_img, out_path);
        gdImageDestroy(out_img);

        gdImageDestroy(in_img);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_thread);
    args->duration = diff_timespec(&end_thread, &start_thread);
    
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    clock_gettime(CLOCK_MONOTONIC, &start_total);
    clock_gettime(CLOCK_MONOTONIC, &start_seq);

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <dir> <num_threads> <-name|-size>\n", argv[0]);
        exit(1);
    }

    char *dir_path = argv[1];
    int n_threads = atoi(argv[2]);
    char *sort_mode = argv[3];

    // Criar diretoria de resultados
    char res_dir_full[512];
    sprintf(res_dir_full, "%s/%s", dir_path, RESULT_IMAGE_DIR);
    create_directory(res_dir_full);

    // Leitura da diretoria
    DIR *d = opendir(dir_path);
    if (!d) {
        perror("Erro ao abrir diretoria");
        exit(1);
    }

    struct dirent *dir;
    struct stat file_stat;
    ImageInfo *images = NULL;
    int n_images = 0;
    int capacity = 10;
    
    images = malloc(sizeof(ImageInfo) * capacity);//aloca inicialmente espaço para 10 imagens

    char full_path[512];

    while ((dir = readdir(d)) != NULL) { 
        char *dot = strrchr(dir->d_name, '.');
        if (dot && strcmp(dot, ".jpeg") == 0) { // Filtra apenas ficheiros .jpeg
            
            if (n_images >= capacity) {
                capacity *= 2;
                images = realloc(images, sizeof(ImageInfo) * capacity);
            }

            strcpy(images[n_images].name, dir->d_name);
            
            sprintf(full_path, "%s/%s", dir_path, dir->d_name);//
            if (stat(full_path, &file_stat) == 0) {
                images[n_images].size = file_stat.st_size;
            } else {
                images[n_images].size = 0;
            }
            n_images++;
        }
    }
    closedir(d);

    // Ordenação 
    if (strcmp(sort_mode, "-name") == 0) {
        qsort(images, n_images, sizeof(ImageInfo), compare_name);
    } else if (strcmp(sort_mode, "-size") == 0) {
        qsort(images, n_images, sizeof(ImageInfo), compare_size);
    } else {
        qsort(images, n_images, sizeof(ImageInfo), compare_name); //default é -name
    }

    clock_gettime(CLOCK_MONOTONIC, &end_seq);

    // Criação e gestão das threads
    pthread_t threads[n_threads];
    ThreadArgs thread_args[n_threads];

    int chunk_size = n_images / n_threads;
    int remainder = n_images % n_threads;
    int start = 0;

    for (int i = 0; i < n_threads; i++) {
        thread_args[i].thread_id = i;
        thread_args[i].images = images;
        thread_args[i].input_dir = dir_path;
        thread_args[i].start_index = start;
        
        // Distribui o que sobra entre as primeiras threads
        int extra = (i < remainder) ? 1 : 0;
        int count = chunk_size + extra;
        
        thread_args[i].end_index = start + count; 
        start += count;

        pthread_create(&threads[i], NULL, worker_thread, &thread_args[i]); 
    }

    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_total);

    // Output dos tempos
    char time_filename[128];
    char mode_clean[10];
    
    if (sort_mode[0] == '-') strcpy(mode_clean, sort_mode + 1);
    else strcpy(mode_clean, sort_mode);

    sprintf(time_filename, "%s/timing_%d-%s.txt", dir_path, n_threads, mode_clean);
    FILE *f_time = fopen(time_filename, "w");
    
    if (f_time) {
        struct timespec total = diff_timespec(&end_total, &start_total);
        struct timespec seq = diff_timespec(&end_seq, &start_seq);
        
        fprintf(f_time, "Total: \t %10jd.%09ld\n", total.tv_sec, total.tv_nsec);
        fprintf(f_time, "Serial:\t %10jd.%09ld\n", seq.tv_sec, seq.tv_nsec);
        
        for (int i = 0; i < n_threads; i++) {
            fprintf(f_time, "Thread %d: %10jd.%09ld\n", i, thread_args[i].duration.tv_sec, thread_args[i].duration.tv_nsec);
        }
        fclose(f_time);
    }

    free(images);
    return 0;
}