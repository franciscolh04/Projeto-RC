#ifndef STATE_H
#define STATE_H

#include <time.h>
#include <math.h>

#define BUF_SIZE 1024

#define FLAG_END 0
#define FLAG_START 1

#define FINALIZED_GAME 0
#define ACTIVE_GAME 1

#define TRIAL_LINE_SIZE 17

// Funções relacionadas com jogos ativos
int has_active_game(int plid, int flag); // Verifica se o jogador tem jogo ativo
void create_game(int plid, const char *secret_code, int max_time, char mode);
int check_trial(int plid, const char *attempt);
int get_last_trial(int plid, int *nT, int *nB, int *nW);
void save_play(int plid, const char *attempt, int nB, int nW, int time_elapsed);
void save_score(int plid, int score, time_t end_time, const char *secret_code, int num_plays, char mode);
int get_secret_code(int plid, char* secret_code);
int get_start_time(int plid, time_t* start_time);
int get_max_playtime(int plid, time_t* max_playtime);
int get_game_mode(int plid, char* mode);
int close_game(int plid, int total_time, char end_code);
int FindLastGame(char *PLID, char *fname);
void format_show_trials(const char *plid, const char *fname, char *buffer, int game_status);

// Diretórios
#define GAMES_DIR "./src/server/GAMES/"
#define SCORES_DIR "./src/server/SCORES/"
#define GAMES_DIR_PATH_LEN 20
#define SCORES_DIR_PATH_LEN 21

#endif // STATE_H
