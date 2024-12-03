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
    if (has_active_game(plid, FLAG_START)) {
        return "RSG NOK\n"; // O jogador já tem um jogo em andamento
    }

    // Criar o código secreto para o novo jogo
    char secret_code[CODE_SIZE + 1];
    generateCode(secret_code);

    // Criar o ficheiro de jogo
    create_game(plid, secret_code, time, 'P');

    // Responder com sucesso
    return "RSG OK\n";
}


const char* handle_try(const char* request) {
    // Parsing
    // Verificar a sintaxe do comando - ERR 
    // Ver se o jogador tem um jogo ativo - NOK
    // Ver se o tempo de jogo foi excedido. Nesse caso, terminar o jogo - ETM (T)
    // Consultar ficheiro de jogo e ver se número de trials está correto - INV
    // Verificar se o jogador já esgotou todas as trials. Se sim, terminar o jogo - ENT (F)
    // Verificar se a tentativa é repetida - DUP
    // Verificar se é resend - OK (reenviar a resposta anterior)
    // Fazer jogada - OK
    // Verificar se o jogo terminou - WIN. Se sim, terminar o jogo - (W)

    return "RTR OK\n";
}

const char* handle_show_trials(const char* request) {
    return "RST OK\n";
}

const char* handle_scoreboard() {
    return "RSS OK\n";
}

const char* handle_debug(const char* request) {
    int plid, time;
    char c1[2], c2[2], c3[2], c4[2];

    // Verificar a sintaxe do comando
    if (sscanf(request, "DBG %d %d %1s %1s %1s %1s", &plid, &time, c1, c2, c3, c4) != 6) {
        return "RDB ERR\n"; // Erro de sintaxe
    }

    // Verificar PLID e tempo
    if (plid <= 0 || time < 0 || time > 600) {
        return "RDB ERR\n";
    }

    // FALTA VERIFICAR AS CORES

    // Verificar se o jogador já tem um jogo ativo
    if (has_active_game(plid, FLAG_START)) {
        return "RDB NOK\n"; // O jogador já tem um jogo em andamento
    }

    // Associa o código secreto fornecido ao novo jogo
    char secret_code[CODE_SIZE + 1];
    snprintf(secret_code, sizeof(secret_code), "%c%c%c%c", c1[0], c2[0], c3[0], c4[0]);

    // Criar o ficheiro de jogo
    create_game(plid, secret_code, time, 'D');

    // Responder com sucesso
    return "RDB OK\n";
}

const char* handle_quit(const char* request) {
    int plid;

    // Verificar a sintaxe do comando
    if (sscanf(request, "QUT %d", &plid) != 1) {
        return "RQT ERR\n"; // Erro de sintaxe
    }

    // Verificar se o jogador tem um jogo ativo
    if (!has_active_game(plid, FLAG_END)) {
        return "RQT NOK\n";
    }

    // Obter o código secreto do jogo ativo
    char secret_code[CODE_SIZE + 1];
    if (!get_secret_code(plid, secret_code)) {
        return "RQT ERR\n"; // Erro ao obter informações do jogo
    }

    // Obter a hora de início do jogo ativo
    time_t start_time;
    if (!get_start_time(plid, &start_time)) {
        return "RQT ERR\n"; // Erro ao obter a hora de início do jogo
    }

    // Calcula o tempo total de jogo
    time_t now = time(NULL); // Hora atual
    int total_time = (int)difftime(now, start_time); // Calcula a diferença em segundos

    // Finalizar o jogo
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


