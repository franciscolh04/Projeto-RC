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

const char* handle_try(const char* request) {
    return "RTR OK\n";
}

const char* handle_show_trials(const char* request) {
    return "RST OK\n";
}

const char* handle_scoreboard() {
    return "RSS OK\n";
}

const char* handle_debug(const char* request) {
    return "RDB OK\n";
}

const char* handle_quit(const char* request) {
    int plid;
    // Verificar a sintaxe do comando
    if (sscanf(request, "QUT %d ", &plid) != 1) {
        return "RQT ERR\n";
    }

    // Verificar se o jogador tem um jogo ativo
    Player* player = find_player(players_head, plid);
    if (player == NULL) {
        return "RQT ERR\n"; // Jogador não encontrado ou sem jogo ativo
    }
    else if (player->current_game == NULL) {
        return "RQT NOK\n"; // O jogador não tem um jogo ativo
    }

    // Terminar o jogo e responder com sucesso
    Game *game = player->current_game;
    const char *secret_code = game->secret_code;
    end_game(game, player);

    static char response[15];
    // Formatar a resposta com a chave secreta
    snprintf(response, sizeof(response), "RQT OK %c %c %c %c\n", 
             secret_code[0], 
             secret_code[1], 
             secret_code[2], 
             secret_code[3]);
    return response;
}
