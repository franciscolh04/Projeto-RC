#ifndef STATE_H
#define STATE_H

// Estrutura para guardar o estado de um jogador
typedef struct Player {
    int player_id;             // ID único do jogador
    char mode;                 // Modo de jogo: 'P' para "play" ou 'D' para "debug"
    struct Game *current_game; // Jogo atual
    struct Game *last_game;    // Último jogo realizado
    struct Player *next;       // Ponteiro para o próximo jogador (lista ligada)
} Player;


// Estrutura para guardar o estado de um jogo
typedef struct Game {
    int game_id;              // ID único do jogo
    char secret_code[4];      // Código secreto do jogo (ex: ['R', 'G', 'B', 'Y'])
    struct Player *player;    // Jogador que iniciou o jogo
    int trials_count;         // Número de tentativas feitas no jogo
    int is_active;            // Flag que indica se o jogo está ativo (1 - ativo, 0 - terminado)
    struct Game *next;        // Ponteiro para o próximo jogo (lista ligada)
} Game;

// Protótipos das funções
void add_player(Player **players_list, int player_id, char mode);
void start_game(Game **games_list, Player *player, int game_id, int *secret_code);
void end_game(Game *game, Player *player);
Player* find_player(Player *players_list, int player_id);
Game* find_game(Player *players_list);
Game* find_game_by_player(Player *player);

#endif // STATE_H
