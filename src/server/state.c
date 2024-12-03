#include "state.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// Verifica se o jogador tem um jogo ativo
int has_active_game(int plid, int flag) {
    char filename[64];
    snprintf(filename, sizeof(filename), "%sGAME_%d.txt", GAMES_DIR, plid);

    FILE *file = fopen(filename, "r");
    if (!file) return 0; // Retorna 0 se o arquivo não existir ou não puder ser aberto

    // QUIT
    if (flag == 0) {
        fclose(file);
        return 1;
    }

    // START
    char buffer[256];
    int line_count = 0;

    // Lê no máximo duas linhas
    for (int i = 0; i < 2 && fgets(buffer, sizeof(buffer), file); i++) {
        line_count++;
    }

    fclose(file);
    return line_count > 1; // Retorna 1 se houver mais de uma linha
}


// Cria um ficheiro para um novo jogo
void create_game(int plid, const char *secret_code, int max_time, char mode) {
    char filename[64];
    snprintf(filename, sizeof(filename), "%sGAME_%06d.txt", GAMES_DIR, plid);

    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Erro ao criar ficheiro de jogo ativo");
        exit(EXIT_FAILURE);
    }

    // Obter data e hora atual
    time_t now;
    struct tm *start_time;
    char time_str[20];


    printf("vai obter hora\n");
    time(&now);
    printf("hora obtida\n");
    printf("vai obter hora gmt\n");
    start_time = gmtime(&now);
    printf("hora gmt obtida\n");

    // Formatar data e hora juntas: YYYY-MM-DD HH:MM:SS
    snprintf(time_str, sizeof(time_str), "%4d-%02d-%02d %02d:%02d:%02d",
             start_time->tm_year + 1900, 
             start_time->tm_mon + 1,    
             start_time->tm_mday,       
             start_time->tm_hour,       
             start_time->tm_min,        
             start_time->tm_sec); 

    // Escreve a linha inicial no ficheiro no formato correto
    fprintf(file, "%06d %c %s %d %s %ld\n", 
            plid,                          // PLID com 6 dígitos
            mode,                          // Modo: P (Play) ou D (Debug)
            secret_code,                   // Código secreto
            max_time,                      // Tempo máximo em segundos
            time_str,                      // Data e hora de início: YYYY-MM-DD HH:MM:SS
            now                            // Momento de início em segundos (timestamp)
    );

    printf("escreveu no ficheiro\n");

    fclose(file);
}

// Regista uma jogada no ficheiro do jogo ativo
void save_play(int plid, const char *attempt, int nB, int nW, int time_elapsed) {
    char filename[64];
    snprintf(filename, sizeof(filename), "%sGAME_%d.txt", GAMES_DIR, plid);

    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Erro ao abrir ficheiro de jogo ativo");
        exit(EXIT_FAILURE);
    }

    // Escreve a jogada no formato: ATTEMPT nB nW TIME_ELAPSED
    fprintf(file, "%s %d %d %d\n", attempt, nB, nW, time_elapsed);
    fclose(file);
}

int get_secret_code(int plid, char* secret_code) {
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%sGAME_%06d.txt", GAMES_DIR, plid); // Usa o GAMES_DIR definido

    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Erro ao abrir o ficheiro do jogo");
        return 0; // Falha na leitura do ficheiro
    }

    // Lê a primeira linha do ficheiro e extrai o código secreto
    if (fscanf(file, "%*06d %*c %4s", secret_code) != 1) { // Ignora PLID e Modo, lê o código secreto
        fclose(file);
        return 0; // Falha ao ler o código secreto
    }

    fclose(file);
    return 1; // Sucesso
}

int get_start_time(int plid, time_t* start_time) {
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%sGAME_%06d.txt", GAMES_DIR, plid);

    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Erro ao abrir o ficheiro do jogo");
        return 0; // Falha na leitura do ficheiro
    }

    // Lê a linha inicial do ficheiro no formato: PLID M CCCC T YYYY-MM-DD HH:MM:SS s
    long start_timestamp;
    if (fscanf(file, "%*06d %*c %*4s %*d %*s %*s %ld", &start_timestamp) != 1) {
        fclose(file);
        return 0; // Falha ao ler o timestamp
    }

    *start_time = (time_t)start_timestamp; // Converte o timestamp para time_t
    fclose(file);
    return 1; // Sucesso
}

// Fecha o jogo e move o ficheiro para o diretório do jogador
int close_game(int plid, int total_time, char end_code) {
    char filename[64], new_filename[128], player_dir[64], timestamp[32];
    snprintf(filename, sizeof(filename), "%sGAME_%d.txt", GAMES_DIR, plid);

    // Abre o ficheiro do jogo ativo para adicionar as informações de encerramento
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Erro ao abrir ficheiro de jogo ativo para encerrar");
        return 0; // Falha ao abrir o ficheiro
    }

    // Obtém a data e hora atuais
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    // Adiciona a linha de encerramento com data, hora e duração
    fprintf(file, "%s %d\n", timestamp, total_time);
    fclose(file);

    // Criar o diretório do jogador (se não existir)
    snprintf(player_dir, sizeof(player_dir), "%s%d", GAMES_DIR, plid);
    if (mkdir(player_dir, 0755) != 0 && errno != EEXIST) {
        perror("Erro ao criar diretório do jogador");
        return 0; // Falha ao criar o diretório
    }

    // Formatar o nome do ficheiro final no diretório do jogador
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", t);
    snprintf(new_filename, sizeof(new_filename), "%s%d/%s_%c.txt", 
             GAMES_DIR, plid, timestamp, end_code);

    // Move o ficheiro para o diretório final
    if (rename(filename, new_filename) != 0) {
        perror("Erro ao mover ficheiro de jogo ativo para diretório final");
        return 0; // Falha ao mover o ficheiro
    }

    return 1; // Sucesso
}
