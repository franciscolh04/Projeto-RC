#ifndef STATE_H
#define STATE_H

#include <time.h>

// Funções relacionadas com jogos ativos
int has_active_game(int plid); // Verifica se o jogador tem jogo ativo
void create_game(int plid, const char *secret_code, int max_time, char mode);
void save_play(int plid, const char *attempt, int nB, int nW, int time_elapsed);
int get_secret_code(int plid, char* secret_code);
int get_start_time(int plid, time_t* start_time);
int close_game(int plid, int total_time, char end_code);

// Diretórios
#define GAMES_DIR "./src/server/GAMES/"
#define SCORES_DIR "./src/server/SCORES/"

#endif // STATE_H
