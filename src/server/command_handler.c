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

    // Verificar se o jogador já tem um jogo ativo
    if (has_active_game(plid)) {
        return "RSG NOK\n"; // O jogador já tem um jogo em andamento
    }

    // Criar o código secreto para o novo jogo
    char *secret_code = (char *)malloc(CODE_SIZE * sizeof(char));
    generateCode(secret_code);

    // Criar o ficheiro do jogo ativo
    create_game(plid, secret_code, time, 'P');

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
    if (sscanf(request, "QUT %d", &plid) != 1) {
        return "RQT ERR\n"; // Erro de sintaxe
    }

    // Verificar se o jogador tem um jogo ativo
    if (!has_active_game(plid)) {
        return "RQT NOK\n";
    }

    // Obter o código secreto do jogo ativo
    char secret_code[CODE_SIZE + 1];
    if (!get_secret_code(plid, secret_code)) {
        return "RQT ERR\n"; // Erro ao obter informações do jogo
    }

    // Finalizar o jogo
    int total_time = 300; // Exemplo de tempo total; ajustar conforme necessário
    if (!close_game(plid, total_time, 'Q')) { // Passa 'Q' como código de encerramento
        return "RQT ERR\n"; // Erro ao terminar o jogo
    }

    // Formatar a resposta com sucesso e o código secreto
    static char response[32];
    snprintf(response, sizeof(response), "RQT OK %c %c %c %c\n",
             secret_code[0], 
             secret_code[1], 
             secret_code[2], 
             secret_code[3]);

    return response;
}


