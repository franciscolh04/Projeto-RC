#ifndef STATE_H
#define STATE_H

#include <time.h>
#include <math.h>
#include "game.h"
#include "./../common/verifications.h"

#define FLAG_END 0
#define FLAG_START 1

#define FINALIZED_GAME 0
#define ACTIVE_GAME 1

#define MODE_PLAY 0
#define MODE_DEBUG 1

#define TRIAL_LINE_SIZE 17
#define SCOREBOAD_SIZE 10

typedef struct {
    int score[SCOREBOAD_SIZE];               // Scores dos jogadores
    char PLID[SCOREBOAD_SIZE][7];   // IDs dos jogadores (strings)
    char col_code[SCOREBOAD_SIZE][CODE_SIZE]; // Códigos das cores (strings)
    int no_tries[SCOREBOAD_SIZE];            // Número de tentativas por jogador
    int mode[SCOREBOAD_SIZE];                // Modo de jogo (PLAY ou DEBUG)
    int n_scores;                        // Número total de scores armazenados
} SCORELIST;

// Funções relacionadas com jogos ativos
int has_active_game(char *plid, int flag); // Verifica se o jogador tem jogo ativo
void create_game(char *plid, const char *secret_code, int max_time, char mode);
int check_trial(char *plid, const char *attempt);
int get_last_trial(char *plid, int *nT, int *nB, int *nW);
void save_play(char *plid, const char *attempt, int nB, int nW, int time_elapsed);
void save_score(char *plid, int score, time_t end_time, const char *secret_code, int num_plays, char mode);
int get_secret_code(char *plid, char* secret_code);
int get_start_time(char *plid, time_t* start_time);
int get_max_playtime(char *plid, time_t* max_playtime);
int get_game_mode(char *plid, char* mode);
int close_game(char *plid, int total_time, char end_code);
int FindLastGame(char *PLID, char *fname);
int FindTopScores(SCORELIST *list);
void format_show_trials(const char *plid, const char *fname, char *buffer, int game_status);
void format_scoreboard(SCORELIST *list, char *buffer);

// Diretórios
#define GAMES_DIR "./src/server/GAMES/"
#define SCORES_DIR "./src/server/SCORES/"
#define GAMES_DIR_PATH_LEN 20
#define SCORES_DIR_PATH_LEN 21

#endif // STATE_H
