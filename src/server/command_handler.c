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
        // Verificar se o tempo já passou. Se sim, terminar o jogo

        // Caso contrário, responder com NOK
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
    int plid, num_trials, exp_num_trials;
    char c1[2], c2[2], c3[2], c4[2], color_code[CODE_SIZE + 1], secret_code[CODE_SIZE + 1];
    printf("entrou no handle_try\n");
    
    // Verificar a sintaxe do comando - ERR
    if (sscanf(request, "TRY %d %1s %1s %1s %1s %d\n", &plid, c1, c2, c3, c4, &num_trials) != 6) {
        return "RTR ERR\n"; // Erro de sintaxe
    }

    // Verificar PLID e cores // FALTA VERIFICAR AS CORES
    if (plid <= 0) {
        return "RSG ERR\n";
    }

    // Verificar se o jogador tem um jogo ativo - NOK
    if (!has_active_game(plid, FLAG_END)) {
        return "RTR NOK\n";
    }

    // Ver se o tempo de jogo foi excedido. Nesse caso, terminar o jogo - ETM (T)
    // Chamar função que faz diferença de tempo ou fazer diretamente aqui?
    time_t start_time, max_playtime, now;
    get_start_time(plid, &start_time);
    get_max_playtime(plid, &max_playtime);

    if (time(&now) > start_time + max_playtime) {
        close_game(plid, max_playtime, 'T');
        return "RTR ETM\n"; // CORRIGIR
    }

    // Verificar se o número de trials é válido
    exp_num_trials = has_active_game(plid, FLAG_START) + 1;
    snprintf(color_code, sizeof(color_code), "%s%s%s%s", c1, c2, c3, c4);
    printf("exp_num_trials: %d\n", exp_num_trials);
    if (exp_num_trials != num_trials) {
        if (check_trial(plid, color_code) == exp_num_trials - 1) {
            // Obter nT, nB e nW do trial anterior
            int nT, nB, nW;
            static char response[13];
            get_last_trial(plid, &nT, &nB, &nW);
            snprintf(response, sizeof(response), "RTR OK %d %d %d\n", nT, nB, nW);
            return response;
        }
        return "RTR INV\n"; // INV
    }

    // Verificar se a tentativa é repetida 
    if (check_trial(plid, color_code) != 0) {
        return "RTR DUP\n"; // DUP
    }

    // Fazer jogada - OK
    int nB, nW;
    get_secret_code(plid, secret_code);
    checkCode(secret_code, color_code, &nB, &nW);
    save_play(plid, color_code, nB, nW, now - start_time);


    // Verificar se o jogador ganhou - Se sim, terminar o jogo - (W)
    static char response[13];
    snprintf(response, sizeof(response), "RTR OK %d %d %d\n", num_trials, nB, nW); // OK
    if (nB == CODE_SIZE) {
        int score = 100;
        char mode;
        
        score = (num_trials == MAX_PLAYS) ? 1 : 
        (num_trials > 1 ? score - (int)floor(num_trials * (100.0 / MAX_PLAYS)) : score);

        get_game_mode(plid, &mode);

        save_score(plid, score, now, secret_code, num_trials, mode);
        close_game(plid, now - start_time, 'W');
        return response;
    }

    // Verificar se o jogador já esgotou todas as trials. Se sim, terminar o jogo - ENT (F)
    if (num_trials == MAX_PLAYS) {
        static char response_fail[16];
        close_game(plid, now - start_time, 'F');
        snprintf(response_fail, sizeof(response_fail), "RTR ENT %c %c %c %c\n", secret_code[0], secret_code[1], secret_code[2], secret_code[3]); // ENT
        return response_fail;
    }
    
    return response;
}

const char* handle_show_trials(const char* request) {
    int plid;
    char buffer[BUF_SIZE];
    static char response[BUF_SIZE];
    char fname[64], PLID[7];
    size_t buffer_size;

    // Verificar a sintaxe do comando
    if (sscanf(request, "STR %d", &plid) != 1) {
        return "RST NOK\n"; // Erro de sintaxe
    }

    // Verificar o número de PLID
    if (plid <= 0) { // FALTAM COISAS
        return "RST NOK\n";
    }

    // Format PLID como string com 6 dígitos
    snprintf(PLID, sizeof(PLID), "%06d", plid);

    // Determinar o ficheiro correspondente (ativo ou terminado)
    if (has_active_game(plid, FLAG_END)) {
        // Jogo ativo
        snprintf(fname, sizeof(fname), "%sGAME_%s.txt", GAMES_DIR, PLID);
        format_show_trials(PLID, fname, buffer, ACTIVE_GAME); // Formata os dados do jogo ativo
        printf("buffer: %s\n", buffer);
        // Formatar a resposta
        buffer_size = strlen(buffer);
        snprintf(response, sizeof(response), "RST ACT STATE_%s.txt %ld\n%s\n", PLID, buffer_size, buffer);
    } else {
        // Último jogo terminado
        if (FindLastGame(PLID, fname) == 0) {
            return "RST NOK\n"; // Nenhum jogo encontrado
        }
        format_show_trials(PLID, fname, buffer, FINALIZED_GAME); // Formata os dados do último jogo terminado
        printf("buffer: %s\n", buffer);
        // Formatar a resposta
        buffer_size = strlen(buffer);
        snprintf(response, sizeof(response), "RST FIN STATE_%s.txt %ld\n%s\n", PLID, buffer_size, buffer);
    }

    return response;
}

const char* handle_scoreboard() {
    SCORELIST list;
    char buffer[BUF_SIZE];
    static char response[BUF_SIZE];
    size_t buffer_size;
    time_t now;
    time(&now);
    struct tm *start_time;
    start_time = gmtime(&now);

    if (FindTopScores(&list) == 0) {
        return "RSS EMPTY\n"; // Nenhum score encontrado
    }

    // Formatar a resposta e retornar
    snprintf(buffer, BUF_SIZE, ""); // Inicializa o buffer vazio.
    format_scoreboard(&list, buffer);
    printf("buffer: %s\n", buffer);
    buffer_size = strlen(buffer);
    snprintf(response, sizeof(response), "RSS OK TOPSCORES_%ld.txt %ld\n%s\n", now, buffer_size, buffer);

    return response;
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
        // Verificar se o tempo já passou. Se sim, terminar o jogo

        // Caso contrário, responder com NOK
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


