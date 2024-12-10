#ifndef STATE_H
#define STATE_H

#include <time.h>

#define FLAG_END 0
#define FLAG_START 1
#define TRIAL_LINE_SIZE 17

// Funções relacionadas com jogos ativos
int has_active_game(int plid, int flag); // Verifica se o jogador tem jogo ativo
void create_game(int plid, const char *secret_code, int max_time, char mode);
int check_trial(int plid, const char *attempt);
int get_last_trial(int plid, int *nT, int *nB, int *nW);
void save_play(int plid, const char *attempt, int nB, int nW, int time_elapsed);
int get_secret_code(int plid, char* secret_code);
int get_start_time(int plid, time_t* start_time);
int get_max_playtime(int plid, time_t* max_playtime);
int close_game(int plid, int total_time, char end_code);

// Diretórios
#define GAMES_DIR "./src/server/GAMES/"
#define SCORES_DIR "./src/server/SCORES/"

#endif // STATE_H
