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
 * Projeto - Parte B
 *
 * Grupo: 122
 * Aluno: Luis Miguel (100010)
 *
 * Ficheiro: process-photos-parallel-B.c
 * Descrição: Versão paralela do processamento de imagens usando Pthreads com Pipes e interatividade.
 *****************************************************************************/

// Estrutura para enviar dados do Main para as Threads via Pipe
typedef struct {
    char dir_path[256];
    char file_name[256];
} Task; //comunicação via Pipe

// Estrutura para armazenar info para ordenação (usado apenas no main agora)
typedef struct {
    char name[256];
    off_t size;
} ImageInfo;

typedef struct {
    int thread_id;
    int pipe_read_fd; //Thread recebe o descritor de leitura do pipe
} ThreadArgs;

// Variáveis Globais para Estatísticas e Sincronização
int total_images_processed = 0;      
double total_processing_time = 0.0;  
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER; //Mutex para proteger as estatísticas

// Funções de comparação 
int compare_name(const void *a, const void *b) {
    ImageInfo *imgA = (ImageInfo *)a;
    ImageInfo *imgB = (ImageInfo *)b;
    return strcmp(imgA->name, imgB->name);
}

int compare_size(const void *a, const void *b) {
    ImageInfo *imgA = (ImageInfo *)a;
    ImageInfo *imgB = (ImageInfo *)b;
    if (imgA->size < imgB->size) return -1;
    if (imgA->size > imgB->size) return 1;
    return strcmp(imgA->name, imgB->name);
}

// Função Worker 
void *worker_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    Task task; //Variável para receber a tarefa do pipe

    // Ciclo infinito: lê do pipe enquanto houver dados (retorno > 0)
    // Se read retornar 0, significa que o main fechou o pipe (sinal de QUIT)
    while (read(args->pipe_read_fd, &task, sizeof(Task)) > 0) { //ler do pipe
        
        struct timespec start_img, end_img;
        clock_gettime(CLOCK_MONOTONIC, &start_img);

        char in_path[512]; 
        char out_path[512];
        char res_dir_path[512];

        // Constrói caminhos baseados na tarefa recebida
        snprintf(res_dir_path, sizeof(res_dir_path), "%s/%s/", task.dir_path, RESULT_IMAGE_DIR);
        snprintf(in_path, sizeof(in_path), "%s/%s", task.dir_path, task.file_name);

        if (access(in_path, R_OK) != 0) continue;

        gdImagePtr in_img = read_jpeg_file(in_path);
        if (!in_img) continue;

        gdImagePtr out_img;

        // --- Processamento das 5 transformações  ---
        // Contrast
        snprintf(out_path, sizeof(out_path), "%scontrast_%s", res_dir_path, task.file_name);
        if (access(out_path, F_OK) != 0) {
            out_img = contrast_image(in_img);
            if (out_img) { write_jpeg_file(out_img, out_path); gdImageDestroy(out_img); }
        }
        // Blur
        snprintf(out_path, sizeof(out_path), "%sblur_%s", res_dir_path, task.file_name);
        if (access(out_path, F_OK) != 0) {
            out_img = blur_image(in_img);
            if (out_img) { write_jpeg_file(out_img, out_path); gdImageDestroy(out_img); }
        }
        // Sepia
        snprintf(out_path, sizeof(out_path), "%ssepia_%s", res_dir_path, task.file_name);
        if (access(out_path, F_OK) != 0) {
            out_img = sepia_image(in_img);
            if (out_img) { write_jpeg_file(out_img, out_path); gdImageDestroy(out_img); }
        }
        // Thumb
        snprintf(out_path, sizeof(out_path), "%sthumb_%s", res_dir_path, task.file_name);
        if (access(out_path, F_OK) != 0) {
            out_img = thumb_image(in_img);
            if (out_img) { write_jpeg_file(out_img, out_path); gdImageDestroy(out_img); }
        }
        // Gray
        snprintf(out_path, sizeof(out_path), "%sgray_%s", res_dir_path, task.file_name);
        if (access(out_path, F_OK) != 0) {
            out_img = gray_image(in_img);
            if (out_img) { write_jpeg_file(out_img, out_path); gdImageDestroy(out_img); }
        }

        gdImageDestroy(in_img);

        // --- Fim processamento ---

        clock_gettime(CLOCK_MONOTONIC, &end_img);
        struct timespec duration = diff_timespec(&end_img, &start_img);
        double seconds = duration.tv_sec + duration.tv_nsec / 1e9;

        // Atualizar Estatísticas 
        pthread_mutex_lock(&stats_mutex);
        total_images_processed++;
        total_processing_time += seconds;
        
        int total_local = total_images_processed;
        double time_local = total_processing_time;
        pthread_mutex_unlock(&stats_mutex); 

        printf("\tthread %d processou %s em %.2fs\n", args->thread_id, task.file_name, seconds);
        printf("\tNumero total de imagens processadas: %d\n", total_local);
        if (total_local > 0)
            printf("\tTempo médio de processamento: %.2fs\n", time_local / total_local);
    }

    close(args->pipe_read_fd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    // Validação de argumentos inicial
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <num_threads> <-name|-size>\n", argv[0]); 
        exit(1);
    }

    int n_threads = atoi(argv[1]);
    char *sort_mode = argv[2];

    // Configuração dos Pipes e Threads
    pthread_t threads[n_threads];
    ThreadArgs thread_args[n_threads];
    int pipes[n_threads][2]; //Array de pipes 

    printf("Foram criadas %d threads\n", n_threads);

    for (int i = 0; i < n_threads; i++) {
        if (pipe(pipes[i]) != 0) {
            perror("Erro ao criar pipe");
            exit(1);
        }
        thread_args[i].thread_id = i;
        thread_args[i].pipe_read_fd = pipes[i][0]; //Passa o lado de leitura para a thread
        
        pthread_create(&threads[i], NULL, worker_thread, &thread_args[i]);
    }

    // Variáveis para leitura de comandos
    char linha[100];
    char comando[100], argumento[100];
    int current_thread_idx = 0; // Para Round-Robin

    // Ciclo principal de leitura de comandos
    while (1) { //Ciclo infinito interativo
        printf("Qual o comando: ");
        if (fgets(linha, 100, stdin) == NULL) break;

        // Resetar buffers
        comando[0] = '\0';
        argumento[0] = '\0';

        int n_palavras = sscanf(linha, "%s %s", comando, argumento);
        if (n_palavras < 1) continue;

        // --- Comando DIR ---
        if (strcmp(comando, "DIR") == 0) {
            if (n_palavras != 2) {
                printf("Erro: DIR requer um argumento (caminho).\n");
                continue;
            }

            DIR *d = opendir(argumento);
            if (!d) {
                printf("Erro: Não foi possível abrir diretoria %s\n", argumento);
                continue;
            }

            // Criar diretoria de resultados 
            char res_dir_full[512];
            snprintf(res_dir_full, sizeof(res_dir_full), "%s/%s", argumento, RESULT_IMAGE_DIR);
            create_directory(res_dir_full);

            // Ler e Ordenar ficheiros 
            struct dirent *dir;
            struct stat file_stat;
            ImageInfo *images = NULL;
            int n_images = 0;
            int capacity = 10;
            images = malloc(sizeof(ImageInfo) * capacity);

            char full_path[512];
            while ((dir = readdir(d)) != NULL) {
                char *dot = strrchr(dir->d_name, '.');
                if (dot && strcmp(dot, ".jpeg") == 0) {
                    snprintf(full_path, sizeof(full_path), "%s/%s", argumento, dir->d_name);
                    if (stat(full_path, &file_stat) == 0 && file_stat.st_size > 0) {
                        if (n_images >= capacity) {
                            capacity *= 2;
                            images = realloc(images, sizeof(ImageInfo) * capacity);
                        }
                        strncpy(images[n_images].name, dir->d_name, 255);
                        images[n_images].name[255] = '\0';
                        images[n_images].size = file_stat.st_size;
                        n_images++;
                    }
                }
            }
            closedir(d);

            // Ordenar
            if (strcmp(sort_mode, "-name") == 0) qsort(images, n_images, sizeof(ImageInfo), compare_name);
            else if (strcmp(sort_mode, "-size") == 0) qsort(images, n_images, sizeof(ImageInfo), compare_size);
            else qsort(images, n_images, sizeof(ImageInfo), compare_name);

            printf("A %d imagens na pasta %s serão processadas pelas %d threads\n", n_images, argumento, n_threads);

            // Enviar tarefas para os pipes (Round-Robin)
            for (int i = 0; i < n_images; i++) {
                Task t;
                strncpy(t.dir_path, argumento, 255);
                strncpy(t.file_name, images[i].name, 255);
                
                // Escreve no pipe da thread atual
                write(pipes[current_thread_idx][1], &t, sizeof(Task)); 
                
                // Avança para a próxima thread
                current_thread_idx = (current_thread_idx + 1) % n_threads;
            }
            free(images); // Liberta a lista temporária, o trabalho já foi enviado
        }
        // --- Comando STAT ---
        else if (strcmp(comando, "STAT") == 0) {
            pthread_mutex_lock(&stats_mutex); //<----- Bloqueia para ler
            printf("Numero total de imagens processadas: %d\n", total_images_processed);
            if (total_images_processed > 0)
                printf("Tempo médio de processamento: %.2fs\n", total_processing_time / total_images_processed);
            else
                printf("Tempo médio de processamento: 0.0s\n");
            pthread_mutex_unlock(&stats_mutex);
        }
        // --- Comando QUIT ---
        else if (strcmp(comando, "QUIT") == 0) {
            break; // Sai do ciclo while(1)
        } 
        else {
            printf("Comando inválido\n");
        }
    }

    // --- Encerramento  ---
    
    // Fecha os pipes de escrita do main.
    for (int i = 0; i < n_threads; i++) {
        close(pipes[i][1]); //retorna 0 
    }

    // Espera todas as threads terminarem o trabalho pendente
    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Estatísticas finais antes de sair
    pthread_mutex_lock(&stats_mutex);
    printf("Numero total de imagens processadas: %d\n", total_images_processed);
    if (total_images_processed > 0)
        printf("Tempo médio de processamento: %.2fs\n", total_processing_time / total_images_processed);
    else
        printf("Tempo médio de processamento: 0.0s\n");
    pthread_mutex_unlock(&stats_mutex);

    return 0;
}