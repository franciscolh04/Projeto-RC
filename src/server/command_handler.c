#include "command_handler.h"
#include "state.h"
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* handle_start(const char* request) {
    int plid, time;

    // Verificar a sintaxe do comando
    if (sscanf(request, "SNG %d %d", &plid, &time) != 2) {
        return "RSG ERR\n"; // Erro de sintaxe
    }

    // Verificar PLID e tempo
    if (plid <= 0 || time < 0 || time > 600) {
        return "RSG ERR\n";
    }

    // Verificar se o jogador já existe
    Player* player = find_player(players_head, plid);
    if (player == NULL) {
        // Adicionar novo jogador se não existir
        add_player(&players_head, plid, 'P');
        player = find_player(players_head, plid);
    }

    // Verificar se o jogador já tem um jogo ativo
    if (player->current_game != NULL && player->current_game->is_active) {
        return "RSG NOK\n"; // O jogador já tem um jogo em andamento
    }

    // Criar um novo jogo
    char *secret_code = (char *)malloc(CODE_SIZE * sizeof(char));
    generateCode(secret_code); // Gerar código secreto aleatório
    start_game(&games_head, player, secret_code); // Criar o jogo com um ID aleatório
    printf("code: %s\n", secret_code);
    free(secret_code);

    // Responder com sucesso
    return "RSG OK\n";
}

const char* handle_try() {
    return "RTR OK\n";
}

const char* handle_show_trials() {
    return "RST OK\n";
}

const char* handle_scoreboard() {
    return "RSS OK\n";
}

const char* handle_debug() {
    return "RDB OK\n";
}

const char* handle_quit() {
    return "RQT OK\n";
}
