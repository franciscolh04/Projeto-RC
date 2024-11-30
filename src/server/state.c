#include "state.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void add_player(Player **players_list, int player_id, char mode) {
    Player *new_player = (Player *)malloc(sizeof(Player));
    new_player->player_id = player_id;
    new_player->mode = mode;  // Atribui o modo de jogo diretamente
    new_player->current_game = NULL;  // Nenhum jogo associado inicialmente
    new_player->last_game = NULL;  // Nenhum jogo anterior
    new_player->next = *players_list; // Adiciona ao início da lista
    *players_list = new_player;
}


// Função para iniciar um novo jogo
void start_game(Game **games_list, Player *player, char *secret_code) {
    Game *new_game = (Game *)malloc(sizeof(Game));
    memcpy(new_game->secret_code, secret_code, sizeof(char) * 4);  // Copia o código secreto
    new_game->player = player;
    new_game->trials_count = 0;
    new_game->is_active = 1; // Jogo ativo
    new_game->next = *games_list; // Adiciona ao início da lista de jogos
    *games_list = new_game;

    // Associa o jogador ao jogo
    player->current_game = new_game;
}


void end_game(Game *game, Player *player) {
    game->is_active = 0;  // Marca o jogo como terminado
    player->current_game = NULL;  // Remove o jogador do jogo
    player->last_game = game;  // Atualiza o último jogo realizado
}


// Função para encontrar um jogador
Player* find_player(Player *players_list, int player_id) {
    Player *current = players_list;
    while (current != NULL) {
        if (current->player_id == player_id) {
            return current;  // Jogador encontrado
        }
        current = current->next;
    }
    return NULL;  // Jogador não encontrado
}

// Função para encontrar o jogo ativo de um jogador
Game* find_game(Player *players_list) {
    Player *current = players_list;
    while (current != NULL) {
        if (current->current_game != NULL && current->current_game->is_active) {
            return current->current_game;  // Jogo ativo encontrado
        }
        current = current->next;
    }
    return NULL;  // Jogo ativo não encontrado
}

// Função para encontrar o jogo ativo de um jogador
Game* find_game_by_player(Player *player) {
    if (player != NULL) {
        return player->current_game;  // Retorna o jogo associado ao jogador
    }
    return NULL;  // Jogador não existe ou não tem jogo ativo
}

