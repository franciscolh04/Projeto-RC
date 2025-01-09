#include "state.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

// Verifica se o jogador tem um jogo ativo e, nesse caso, devolve o número de trials do mesmo
int has_active_game(char *plid, int flag) {
    char filename[64];
    snprintf(filename, sizeof(filename), "%sGAME_%s.txt", GAMES_DIR, plid);

    FILE *file = fopen(filename, "r");
    if (!file) return 0; // Retorna 0 se o arquivo não existir ou não puder ser aberto
    
    // TRY or QUT
    if (flag == 0) {
        fclose(file);
        return 1;
    }

    // SNG
    char buffer[256];
    int line_count = 0;

    // Conta o número de linhas no ficheiro
    while(fgets(buffer, sizeof(buffer), file)) {
        line_count++;
    }

    fclose(file);
    return line_count - 1; // Retorna >0 se houver mais de uma linha
}


// Cria um ficheiro para um novo jogo
void create_game(char *plid, const char *secret_code, int max_time, char mode) {
    char filename[64];
    snprintf(filename, sizeof(filename), "%sGAME_%s.txt", GAMES_DIR, plid);

    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Erro ao criar ficheiro de jogo ativo");
        exit(EXIT_FAILURE);
    }

    // Obter data e hora atual
    time_t now;
    struct tm *start_time;
    char time_str[20];

    time(&now);
    start_time = gmtime(&now);

    // Formatar data e hora juntas: YYYY-MM-DD HH:MM:SS
    snprintf(time_str, sizeof(time_str), "%4d-%02d-%02d %02d:%02d:%02d",
             start_time->tm_year + 1900, 
             start_time->tm_mon + 1,    
             start_time->tm_mday,       
             start_time->tm_hour,       
             start_time->tm_min,        
             start_time->tm_sec); 

    // Escreve a linha inicial no ficheiro no formato correto
    fprintf(file, "%s %c %s %d %s %ld\n", 
            plid,                          // PLID com 6 dígitos
            mode,                          // Modo: P (Play) ou D (Debug)
            secret_code,                   // Código secreto
            max_time,                      // Tempo máximo em segundos
            time_str,                      // Data e hora de início: YYYY-MM-DD HH:MM:SS
            now                            // Momento de início em segundos (timestamp)
    );

    fclose(file);
}

int check_trial(char *plid, const char *attempt) {
    char filename[64];
    snprintf(filename, sizeof(filename), "%sGAME_%s.txt", GAMES_DIR, plid);

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o ficheiro do jogo");
        return 0; // Retorna 0 se o ficheiro não existir
    }

    char line[TRIAL_LINE_SIZE];
    char trial_color_code[CODE_SIZE + 1];
    int trial_counter = 0;

    // Lê o ficheiro linha a linha para encontrar o trial correspondente
    while (fgets(line, sizeof(line), file)) {
        // Verifica se a linha começa com "T: " (indica um trial)
        if (strncmp(line, "T: ", 3) == 0) {
            trial_counter++; // Incrementa o número de trials processados

            // Extrai o código de cores da linha
            if (sscanf(line, "T: %4s", trial_color_code) == 1) {
                if (strcmp(trial_color_code, attempt) == 0) {
                    fclose(file);
                    return trial_counter; // O trial foi encontrado
                }
            }
        }
    }

    fclose(file);
    return 0; // O trial não foi encontrado
}

int get_last_trial(char *plid, int *nT, int *nB, int *nW) {
    char filename[64];

    snprintf(filename, sizeof(filename), "%sGAME_%s.txt", GAMES_DIR, plid);
   
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o ficheiro do jogo");
        return 0;
    }

    char line[TRIAL_LINE_SIZE];
    char color_code[CODE_SIZE + 1];
    int trial_counter = 0;
    int found = 0;

    // Percorre todas as linhas do ficheiro
    while (fgets(line, sizeof(line), file)) {
        // Verifica se a linha começa com "T: " (indica um trial)
        if (strncmp(line, "T: ", 3) == 0) {
            // Incrementa o número do trial
            trial_counter++;

            // Extrai os valores do trial atual
            if (sscanf(line, "T: %4s %d %d %*d", color_code, nB, nW) == 3) {
                found = 1; // Atualiza os valores encontrados
            }
        }
    }

    fclose(file);
    *nT = trial_counter; // Atualiza o número do último trial
    return found; // Retorna 1 se um trial foi encontrado, 0 caso contrário
}


// Regista uma jogada no ficheiro do jogo ativo
void save_play(char *plid, const char *attempt, int nB, int nW, int time_elapsed) {
    char filename[64];
    snprintf(filename, sizeof(filename), "%sGAME_%s.txt", GAMES_DIR, plid);

    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Erro ao abrir ficheiro de jogo ativo");
        exit(EXIT_FAILURE);
    }

    // Escreve a jogada no formato: ATTEMPT nB nW TIME_ELAPSED
    fprintf(file, "T: %s %d %d %d\n", attempt, nB, nW, time_elapsed);
    fclose(file);
}

// Regista a pontuação de um jogador num dado jogo
void save_score(char *plid, int score, time_t now, const char *secret_code, int num_trials, char mode) {
    char filename[SCORES_DIR_PATH_LEN + 31];
    char time_str[20];

    struct tm *end_time;
    end_time = gmtime(&now);

    // Formatar a data e hora de fim
    snprintf(time_str, sizeof(time_str), "%02d%02d%4d_%02d%02d%02d",
             end_time->tm_mday,       
             end_time->tm_mon + 1,    
             end_time->tm_year + 1900, 
             end_time->tm_hour,       
             end_time->tm_min,        
             end_time->tm_sec);

    // Formatar o nome do ficheiro: score_PLID_DDMMYYYY_HHMMSS.txt
    snprintf(filename, sizeof(filename), "%s%03d_%s_%s.txt", 
             SCORES_DIR, score, plid, time_str);

    // Criar e abrir o ficheiro para escrita
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Erro ao criar ficheiro de score");
        exit(EXIT_FAILURE);
    }

    // Escrever a linha com os dados do score
    fprintf(file, "%03d %s %s %d %s\n", 
            score, plid, secret_code, num_trials, (mode == 'P') ? "PLAY" : "DEBUG");

    fclose(file);
}

int get_secret_code(char *plid, char* secret_code) {
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%sGAME_%s.txt", GAMES_DIR, plid); // Usa o GAMES_DIR definido

    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Erro ao abrir o ficheiro do jogo");
        return 0; // Falha na leitura do ficheiro
    }
  
    // Lê a primeira linha do ficheiro e extrai o código secreto
    if (fscanf(file, "%*s %*c %4s", secret_code) != 1) { // Ignora PLID e Modo, lê o código secreto
        fclose(file);
        return 0; // Falha ao ler o código secreto
    }

    fclose(file);
    return 1; // Sucesso
}

int get_start_time(char *plid, time_t* start_time) {
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%sGAME_%s.txt", GAMES_DIR, plid);

    FILE *file = fopen(filepath, "r");

    if (file == NULL) {
        perror("Erro ao abrir o ficheiro do jogo");
        return 0; // Falha na leitura do ficheiro
    }

    // Lê a linha inicial do ficheiro no formato: PLID M CCCC T YYYY-MM-DD HH:MM:SS s
    long start_timestamp;
    if (fscanf(file, "%*6s %*c %*4s %*d %*s %*s %ld", &start_timestamp) != 1) {
        fclose(file);
        return 0; // Falha ao ler o timestamp
    }

    *start_time = (time_t)start_timestamp; // Converte o timestamp para time_t
    fclose(file);
    return 1; // Sucesso
}

int get_max_playtime(char *plid, time_t* max_playtime) {
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%sGAME_%s.txt", GAMES_DIR, plid);
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Erro ao abrir o ficheiro do jogo");
        return 0; // Falha na leitura do ficheiro
    }

    // Lê a linha inicial do ficheiro no formato: PLID M CCCC T YYYY-MM-DD HH:MM:SS s
    int playtime;
    if (fscanf(file, "%*6s %*c %*4s %d", &playtime) != 1) {
        fclose(file);
        return 0; // Falha ao ler o tempo máximo
    }

    *max_playtime = playtime; // Converte o timestamp para time_t
    fclose(file);
    return 1; // Sucesso
}

int get_game_mode(char *plid, char* mode) {
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "%sGAME_%s.txt", GAMES_DIR, plid);

    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Erro ao abrir o ficheiro do jogo");
        return 0; // Falha na leitura do ficheiro
    }

    // Lê a linha inicial do ficheiro no formato: PLID M CCCC T YYYY-MM-DD HH:MM:SS s
    if (fscanf(file, "%*6s %c", mode) != 1) {
        fclose(file);
        return 0; // Falha ao ler o modo de jogo
    }

    fclose(file);
    return 1; // Sucesso
}

// Fecha o jogo e move o ficheiro para o diretório do jogador
int close_game(char *plid, int total_time, char end_code) {
    char filename[64], new_filename[128], player_dir[64], time_str[20];
    snprintf(filename, sizeof(filename), "%sGAME_%s.txt", GAMES_DIR, plid);

    // Abre o ficheiro do jogo ativo para adicionar as informações de encerramento
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Erro ao abrir ficheiro de jogo ativo para encerrar");
        return 0; // Falha ao abrir o ficheiro
    }

    // Obter data e hora de fim de jogo
    struct tm *end_time;

    if (end_code == 'T') {
        time_t start_time, max_playtime, final_time;
        get_start_time(plid, &start_time);
        get_max_playtime(plid, &max_playtime);
        final_time = start_time + max_playtime;
        end_time = gmtime(&final_time);
    } else {
        time_t now;
        time(&now);
        end_time = gmtime(&now);
    }

    // Formatar data e hora juntas: YYYY-MM-DD HH:MM:SS
    snprintf(time_str, sizeof(time_str), "%4d-%02d-%02d %02d:%02d:%02d",
             end_time->tm_year + 1900, 
             end_time->tm_mon + 1,    
             end_time->tm_mday,       
             end_time->tm_hour,       
             end_time->tm_min,        
             end_time->tm_sec);

    // Adiciona a linha de encerramento com data, hora e duração
    fprintf(file, "%s %d\n", time_str, total_time);
    fclose(file);

    // Criar o diretório do jogador (se não existir)
    snprintf(player_dir, sizeof(player_dir), "%s%s", GAMES_DIR, plid);
    if (mkdir(player_dir, 0755) != 0 && errno != EEXIST) {
        perror("Erro ao criar diretório do jogador");
        return 0; // Falha ao criar o diretório
    }

    // Formatar o nome do ficheiro final no diretório do jogador
    snprintf(time_str, sizeof(time_str), "%4d%02d%02d_%02d%02d%02d",
             end_time->tm_year + 1900, 
             end_time->tm_mon + 1,    
             end_time->tm_mday,       
             end_time->tm_hour,       
             end_time->tm_min,        
             end_time->tm_sec);

    snprintf(new_filename, sizeof(new_filename), "%s%s/%s_%c.txt", 
             GAMES_DIR, plid, time_str, end_code);

    // Move o ficheiro para o diretório final
    if (rename(filename, new_filename) != 0) {
        perror("Erro ao mover ficheiro de jogo ativo para diretório final");
        return 0; // Falha ao mover o ficheiro
    }

    return 1; // Sucesso
}

// Função para encontrar o último jogo de um jogador
int FindLastGame(char *PLID, char *fname) {
    struct dirent **filelist;
    int n_entries, found = 0;
    char dirname[GAMES_DIR_PATH_LEN + 7];

    // Formatar o caminho do diretório do jogador
    sprintf(dirname, "%s%s/", GAMES_DIR, PLID);

    // Obter a lista de entradas do diretório
    n_entries = scandir(dirname, &filelist, 0, alphasort);
    if (n_entries <= 0) {
        return 0; // Diretório vazio ou não encontrado
    }

    // Percorrer as entradas do diretório de trás para frente
    while (n_entries--) {
        // Ignorar entradas que começam com '.'
        if (filelist[n_entries]->d_name[0] != '.') {
            // Formatar o caminho completo do ficheiro
            sprintf(fname, "%s%s/%s", GAMES_DIR, PLID, filelist[n_entries]->d_name);
            found = 1;
        }
        free(filelist[n_entries]); // Liberar a memória da entrada atual

        if (found) {
            break; // Encontrou o último jogo
        }
    }

    free(filelist); // Liberar a memória da lista de entradas
    return found; // Retorna 1 se encontrou, 0 caso contrário
}

// Função para encontrar os 10 melhores scores
int FindTopScores(SCORELIST *list) {
    struct dirent **filelist;
    int n_entries, i_file;
    char fname[SCORES_DIR_PATH_LEN + 31];
    FILE *fp;

    n_entries = scandir(SCORES_DIR, &filelist, 0, alphasort);

    i_file = 0;
    if (n_entries < 0) {
        return (0);
    } else {
        while (n_entries--) {
            if (filelist[n_entries]->d_name[0] != '.') {
                sprintf(fname, "%s%s", SCORES_DIR, filelist[n_entries]->d_name);
                fp = fopen(fname, "r");

                if (fp != NULL) {
                    char mode[6];
                    fscanf(fp, "%d %s %s %d %s",
                        &list->score[i_file], list->PLID[i_file], list->col_code[i_file], &list->no_tries[i_file], mode);
                    
                    if (!strcmp(mode, "PLAY")) 
                        list->mode[i_file] = MODE_PLAY;
                    
                    if (!strcmp(mode, "DEBUG")) 
                        list->mode[i_file] = MODE_DEBUG;
                    
                    fclose(fp);
                    ++i_file;
                }
            }

            free(filelist[n_entries]);

            if (i_file == 10)
                break;
        }

        free(filelist);
    }

    list->n_scores = i_file;
    return i_file;
}


// Função para formatar buffer com dados do jogo para o comando show_trials
/*
void format_show_trials(const char *plid, const char *fname, char *buffer, int game_status) {
    FILE *file = fopen(fname, "r");
    if (!file) {
        snprintf(buffer, BUF_SIZE, "Error: Unable to open file for player %s\n", plid);
        return;
    }

    char line[BUF_SIZE];

    int max_time = 0;
    time_t start_time, now;

    buffer[0] = '\0'; // Inicializa o buffer vazio.

    // Lê a linha inicial do ficheiro (metadados do jogo).
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%*6s %*c %*4s %d %*19[^\n] %ld", &max_time, &start_time) != 2) {
        snprintf(buffer, BUF_SIZE, "Error: Invalid file format for player %s\n", plid);
        fclose(file);
        return;
    }


    // Processar as linhas de tentativas.
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "T: ", 3) == 0) {
            int nB, nW;
            char trial_code[CODE_SIZE + 1];
            // Linha de tentativa
            if (sscanf(line, "T: %4s %d %d %*d", trial_code, &nB, &nW) == 3) {
                snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer),
                         "%c %c %c %c %d %d\n",
                         trial_code[0], trial_code[1], trial_code[2], trial_code[3], nB, nW);
            }
        }
    }

    // Calcular o tempo restante para jogos ativos.
    if (game_status == ACTIVE_GAME) {
        time(&now); // Obtém o tempo atual.
        int remaining_time = max_time - (int)difftime(now, start_time);
        if (remaining_time < 0) {
            remaining_time = 0; // Evita valores negativos.
        }
        snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer), "%d\n", remaining_time);
    } else if (game_status == FINALIZED_GAME) {
        snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer), "Game ended\n");
    }

    fclose(file);
}
*/

void format_show_trials(const char *plid, const char *fname, char *buffer, int game_status) {
    FILE *file = fopen(fname, "r");
    if (!file) {
        snprintf(buffer, BUF_SIZE, "Error: Unable to open file for player %s\n", plid);
        return;
    }

    char line[BUF_SIZE];
    char lines_buffer[BUF_SIZE];
    char game_mode, termination_mode, secret_code[CODE_SIZE + 1];
    char start_date[20];
    char end_date[20];
    int max_time = 0, total_time = 0, num_trials = 0;
    time_t start_time, end_time, now;

    time(&now); // Obtém o tempo atual para cálculos.
    snprintf(buffer, BUF_SIZE, ""); // Inicializa o buffer vazio.

    // Processar a primeira linha do ficheiro (dados principais do jogo).
    if (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%s %c %4s %d %19[^\n] %ld", plid, &game_mode, secret_code, &max_time, start_date, &start_time) != 6) {
            snprintf(buffer, BUF_SIZE, "Error: Invalid file format for player %s\n", plid);
            fclose(file);
            return;
        }
    }

    // Formatar a saída inicial.
    if (game_status == ACTIVE_GAME) {
        snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer),
                 "Active game found for player %s\n", plid);
    } else {
        snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer),
                 "Last finalized game for player %s\n", plid);
    }

    snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer),
             "Game initiated: %s with %d seconds to be completed\n",
             start_date, max_time);

    if (game_status == FINALIZED_GAME) {
        snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer),
                 "Mode: %s  Secret code: %s\n", (game_mode == 'P') ? "PLAY" : "DEBUG", secret_code);
    }

    // Processar as linhas de trials.
    snprintf(lines_buffer, BUF_SIZE, ""); // Inicializa o buffer vazio.
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "T: ", 3) == 0) {
            num_trials++;
            char trial_code[CODE_SIZE + 1];
            int nB, nW, time_elapsed;
            sscanf(line, "T: %4s %d %d %d", trial_code, &nB, &nW, &time_elapsed);
            snprintf(lines_buffer + strlen(lines_buffer), BUF_SIZE - strlen(lines_buffer),
                     "Trial: %s, nB: %d, nW: %d   %ds\n", trial_code, nB, nW, time_elapsed);
        } else {
            sscanf(line, "%19[^\n] %d", end_date, &total_time);
        }
    }
    printf("lines_buffer: %s\n", lines_buffer);

    if (num_trials == 0) {
        snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer),
                 "\nGame started - no transactions found\n");
    } else {
        snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer),
             "\n     --- Transactions found: %d ---\n", num_trials);
        snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer), "%s", lines_buffer);
    }

    // Informações finais sobre o jogo.
    if (game_status == ACTIVE_GAME) {
        int remaining_time = max_time - (int)difftime(now, start_time);
        snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer),
                 "\n-- %d seconds remaining to be completed --\n", remaining_time);
    } else {
        termination_mode = *(strrchr(fname, '_') + 1);
        printf("termination_mode: %c\n", termination_mode);
        snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer),
                 "\nTermination: %s at %s, Duration: %ds\n",
                 (termination_mode == 'W') ? "WIN" : ((termination_mode == 'F') ? "FAIL" : ((termination_mode == 'Q') ? "QUIT" : "TIMEOUT")),
                 end_date, total_time);
    }

    fclose(file);
}


// Função para formatar o buffer para a scoreboard
/*
void format_scoreboard(SCORELIST *list, char *buffer) {
    buffer[0] = '\0'; // Inicializar o buffer como vazio.

    // Iterar sobre os scores e formatar cada linha.
    for (int i = 0; i < list->n_scores && i < 10; i++) { // Máximo de 10 linhas.
        char line[128]; // Buffer para cada linha formatada.
        // Formatar a linha com PLID, código secreto e número de tentativas.
        snprintf(line, sizeof(line), 
                "%-6s %4.4s %d\n",      // Separar os campos por espaços, com '\n' no final.
                list->PLID[i],          // Player ID.
                list->col_code[i],      // Código das cores (chave secreta).
                list->no_tries[i]       // Número de tentativas.
        );
        strcat(buffer, line); // Adicionar a linha ao buffer principal.
    }
}
*/

void format_scoreboard(SCORELIST *list, char *buffer) {
    // Formatar o cabeçalho com o número correto de scores
    snprintf(buffer, 128, 
             "----------------------------- TOP %d SCORES -----------------------------\n\n",
             list->n_scores);

    strcat(buffer, "                 SCORE PLAYER     CODE    NO TRIALS   MODE\n\n");

    // Iterar sobre os scores e formatar cada linha
    for (int i = 0; i < list->n_scores; i++) {
        char line[128]; // Buffer para cada linha formatada
        snprintf(line, sizeof(line), 
                 "            %2d -  %3d  %-7s    %-4.4s        %d       %-5s\n",
                 i + 1,                                // Posição na tabela
                 list->score[i],                      // Score
                 list->PLID[i],                       // Player ID
                 list->col_code[i],                   // Código das cores
                 list->no_tries[i],                   // Número de tentativas
                 list->mode[i] == MODE_PLAY ? "PLAY" : "DEBUG" // Modo (PLAY ou DEBUG)
        );
        strcat(buffer, line); // Adicionar a linha ao buffer principal
    }   
}

