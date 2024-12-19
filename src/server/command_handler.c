#include "command_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* handle_start(const char* request) {
    char plid[7];
    int playtime;

    // Verificar a sintaxe do comando
    if (sscanf(request, "SNG %6s %d\n", plid, &playtime) != 2) {
        if (VERBOSE) {
            printf("Syntax error in command\n");
        }
        return "RSG ERR\n"; // Erro de sintaxe
    }

    if (strlen(request) != (15) || request[3] != ' ' || request[10] != ' ' || atoi(plid) <= 100000 || playtime < 0 || playtime > 600) { 
        if (VERBOSE) {
            printf("Syntax error in command\n\n");
        }
        return "RSG ERR\n"; // Erro de sintaxe
    }

    // Verificar se o jogador já tem um jogo ativo
    if (has_active_game(plid, FLAG_START)) {
        // Verificar se o tempo já passou. Se sim, terminar o jogo ativo
        time_t start_time, max_playtime, now;
        get_start_time(plid, &start_time);
        get_max_playtime(plid, &max_playtime);

        if (time(&now) > start_time + max_playtime) {
            if (VERBOSE) {
                printf("PLID=%s: TIMEOUT (game ended)\n", plid);
            }
            close_game(plid, max_playtime, 'T');
        } else { // Caso contrário, responder com NOK
            if (VERBOSE) {
                printf("PLID=%s, player already has an active game\n\n", plid);
            }
            return "RSG NOK\n"; // O jogador já tem um jogo em andamento
        }
    }

    // Criar o código secreto para o novo jogo
    char secret_code[CODE_SIZE + 1];
    generateCode(secret_code);

    // Criar o ficheiro de jogo
    create_game(plid, secret_code, playtime, 'P');

    
    if (VERBOSE) {
        printf("PLID=%s: new game (max %d sec); Colors: %c %c %c %c\n\n", plid, playtime, secret_code[0], secret_code[1], secret_code[2], secret_code[3]);
    }


    // Responder com sucesso
    return "RSG OK\n";
}

const char* handle_try(const char* request) {
    char plid[7];
    int num_trials, exp_num_trials;
    char c1[2], c2[2], c3[2], c4[2], color_code[CODE_SIZE + 1], secret_code[CODE_SIZE + 1];

    
    // Verificar a sintaxe do comando - ERR
    if (sscanf(request, "TRY %6s %1s %1s %1s %1s %d\n", plid, c1, c2, c3, c4, &num_trials) != 6) {
        if (VERBOSE) {
            printf("Syntax error in command\n\n");
        }
        return "RTR ERR\n"; // Erro de sintaxe
    }
    snprintf(color_code, sizeof(color_code), "%s%s%s%s", c1, c2, c3, c4);
    if (!valid_colors(color_code) || strlen(request) != 21 || request[3] != ' '|| request[10] != ' '||request[12] != ' ' || request[14] != ' ' || request[16] != ' ' || request[18] != ' ' || atoi(plid) < 100000 ) {
        if (VERBOSE) {
            printf("Syntax error in command\n\n");
        }
        return "RTR ERR\n"; // Erro de sintaxe
    }

    // Verificar se o jogador tem um jogo ativo - NOK
    if (!has_active_game(plid, FLAG_END)) {
        char fname[64];
        if (FindLastGame(plid, fname)) {
            FILE *file = fopen(fname, "r");
            if (!file) {
                perror("Erro ao abrir o ficheiro do jogo");
                return 0;
            }
            
            char line[TRIAL_LINE_SIZE];
            char last_color_code[CODE_SIZE + 1];
            int trial_counter = 0;
            int found_secret_code = 0;
            int nB = 0, nW = 0;

            // Percorre todas as linhas do ficheiro
            while (fgets(line, sizeof(line), file)) {
                if (!found_secret_code) {
                    sscanf(line, "%*s %*c %4s", secret_code);
                    found_secret_code = 1;
                } 

                // Verifica se a linha começa com "T: " (indica um trial)
                if (strncmp(line, "T: ", 3) == 0) {
                    // Incrementa o número do trial
                    trial_counter++;

                    // Extrai os valores do trial atual
                    sscanf(line, "T: %4s %d %d %*d", last_color_code, &nB, &nW);
                }
            }
            fclose(file);

            if (strcmp(color_code, last_color_code) == 0 && trial_counter == num_trials) {
                static char response[17];
                if (nB == CODE_SIZE) {
                    if (VERBOSE) {
                        printf("RESEND: PLID=%s, try %c %c %c %c - nB = %d, nW = %d: WIN (game ended)\n\n", plid, color_code[0], color_code[1], color_code[2], color_code[3], nB, nW);
                    }
                    snprintf(response, sizeof(response), "RTR OK %d %d %d\n", num_trials, nB, nW); // Resend OK
                } else {
                    if (VERBOSE) {
                        printf("RESEND: PLID=%s, try %c %c %c %c - nB = %d, nW = %d: FAIL (game ended)\n\n", plid, color_code[0], color_code[1], color_code[2], color_code[3], nB, nW);
                    }
                    snprintf(response, sizeof(response), "RTR ENT %c %c %c %c\n", secret_code[0], secret_code[1], secret_code[2], secret_code[3]); // Resend ENT
                }
                return response;
            }

        }
        if (VERBOSE) {
            printf("PLID=%s, player doesn't have an active game\n\n", plid);
        }
        return "RTR NOK\n";
    }


    // Ver se o tempo de jogo foi excedido. Nesse caso, terminar o jogo - ETM (T)
    time_t start_time, max_playtime, now;
    get_start_time(plid, &start_time);
    get_max_playtime(plid, &max_playtime);

    if (time(&now) > start_time + max_playtime) {
        static char response[17];

        get_secret_code(plid, secret_code);
        
        sprintf(response, "RTR ETM %c %c %c %c\n", secret_code[0], secret_code[1], secret_code[2], secret_code[3]);
        close_game(plid, max_playtime, 'T');
        if (VERBOSE) {
            printf("PLID=%s, TIMEOUT (game ended)\n\n", plid);
        }
        return response;
    }

    // Verificar se o número de trials é válido
    exp_num_trials = has_active_game(plid, FLAG_START) + 1;
    if (exp_num_trials != num_trials) {
        if (check_trial(plid, color_code) == exp_num_trials - 1) {
            // Obter nT, nB e nW do trial anterior
            int nT, nB, nW;
            static char response[14];
            get_last_trial(plid, &nT, &nB, &nW);
            snprintf(response, sizeof(response), "RTR OK %d %d %d\n", nT, nB, nW);
            if (VERBOSE) {
                printf("RESEND: PLID=%s: try %c %c %c %c - nB = %d, nW = %d; not guessed\n\n",plid, color_code[0], color_code[1], color_code[2], color_code[3], nB, nW);
            }
            return response;
        }
        if (VERBOSE) {
            printf("PLID=%s, wrong expected number of trials\n\n", plid);
        }
        return "RTR INV\n"; // INV
    }

    // Verificar se a tentativa é repetida 
    if (check_trial(plid, color_code) != 0) {
        if (VERBOSE) {
            printf("PLID=%s, alredy guessed %c %c %c %c\n\n", plid, color_code[0], color_code[1], color_code[2], color_code[3]);
        }
        return "RTR DUP\n"; // DUP
    }
    // Fazer jogada - OK
    int nB, nW;
    get_secret_code(plid, secret_code);
    checkCode(secret_code, color_code, &nB, &nW);
    save_play(plid, color_code, nB, nW, now - start_time);

    // Verificar se o jogador ganhou - Se sim, terminar o jogo - (W)
    static char response[14];
    snprintf(response, sizeof(response), "RTR OK %d %d %d\n", num_trials, nB, nW); // OK
    if (nB == CODE_SIZE) {
        int score = 100;
        char mode;
        
        score = (num_trials == MAX_PLAYS) ? 1 : 
        (num_trials > 1 ? score - (int)floor(num_trials * (100.0 / MAX_PLAYS)) : score);

        get_game_mode(plid, &mode);

        save_score(plid, score, now, secret_code, num_trials, mode);
        close_game(plid, now - start_time, 'W');

        if (VERBOSE) {
            printf("PLID=%s, try %c %c %c %c - nB = %d, nW = %d: WIN (game ended)\n\n", plid, color_code[0], color_code[1], color_code[2], color_code[3], nB, nW);
        }
        return response;
    }

    // Verificar se o jogador já esgotou todas as trials. Se sim, terminar o jogo - ENT (F)
    if (num_trials == MAX_PLAYS) {
        static char response_fail[17];
        close_game(plid, now - start_time, 'F');
        snprintf(response_fail, sizeof(response_fail), "RTR ENT %c %c %c %c\n", secret_code[0], secret_code[1], secret_code[2], secret_code[3]); // ENT
        if (VERBOSE) {
            printf("PLID=%s, try %c %c %c %c - nB = %d, nW = %d: FAIL (game ended)\n\n", plid, color_code[0], color_code[1], color_code[2], color_code[3], nB, nW);
        }
        return response_fail;
    }

    if (VERBOSE) {
        printf("PLID=%s: try %c %c %c %c - nB = %d, nW = %d; not guessed\n\n",plid, color_code[0], color_code[1], color_code[2], color_code[3], nB, nW);
    }

    return response;
}

const char* handle_show_trials(const char* request) {
    char plid[7];
    char buffer[BUF_SIZE];
    static char response[BUF_SIZE];
    char fname[64];
    size_t buffer_size;

    // Verificar a sintaxe do comando
    if (sscanf(request, "STR %s\n", plid) != 1) {
        if (VERBOSE) {
            printf("Syntax error in command\n\n");
        }
        return "RST NOK\n"; // Erro de sintaxe
    }

    if(strlen(request) != 11 || request[3] != ' ' || atoi(plid) < 100000) {
        if (VERBOSE) {
            printf("Syntax error in command\n\n");
        }
        return "RST NOK\n"; // Erro de sintaxe
    }

    // Determinar o ficheiro correspondente (ativo ou terminado)
    if (has_active_game(plid, FLAG_END)) {
        snprintf(fname, sizeof(fname), "%sGAME_%s.txt", GAMES_DIR, plid);

        // Verificar se o tempo já passou. Se sim, terminar o jogo ativo
        time_t start_time, max_playtime, now;
        get_start_time(plid, &start_time);
        get_max_playtime(plid, &max_playtime);

        if (time(&now) > start_time + max_playtime) {
            if (VERBOSE) {
                printf("PLID=%s: TIMEOUT (game ended)\n", plid);
            }
            close_game(plid, max_playtime, 'T'); // Termina o jogo
        } else { 
            // Jogo ativo
            format_show_trials(plid, fname, buffer, ACTIVE_GAME); // Formata os dados como jogo ativo
            buffer_size = strlen(buffer);
            snprintf(response, sizeof(response), "RST ACT STATE_%s.txt %ld %s\n", plid, buffer_size, buffer);
            if (VERBOSE) {
                printf("PLID=%s: show trials: \"%s\" (%ld bytes)\n\n", plid, fname, buffer_size);
            }
            return response;
        }
    }
    // Último jogo terminado
    if (FindLastGame(plid, fname) == 0) {
        if (VERBOSE) {
            printf("PLID=%s, player hasn't played any game\n\n", plid);
        }
        return "RST NOK\n"; // Nenhum jogo encontrado
    }
    format_show_trials(plid, fname, buffer, FINALIZED_GAME); // Formata os dados do último jogo terminado
    buffer_size = strlen(buffer);
    snprintf(response, sizeof(response), "RST FIN STATE_%s.txt %ld %s\n", plid, buffer_size, buffer);
    
    if (VERBOSE) {
        printf("PLID=%s: show trials: \"%s\" (%ld bytes)\n\n", plid, fname, buffer_size);
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

    if (FindTopScores(&list) == 0) {
        if (VERBOSE) {
            printf("No scores found\n\n");
        }
        return "RSS EMPTY\n"; // Nenhum score encontrado
    }

    // Formatar a resposta e retornar
    format_scoreboard(&list, buffer);
    buffer_size = strlen(buffer);
    snprintf(response, sizeof(response), "RSS OK TOPSCORES_%ld.txt %ld %s\n", now, buffer_size, buffer);

    if (VERBOSE) {
        printf("Send scoreboard file \"TOPSCORES_%ld.txt\" (%ld bytes)\n\n", now, buffer_size);
    }

    return response;
}

const char* handle_debug(const char* request) {
    char plid[7];
    int playtime;
    char c1[2], c2[2], c3[2], c4[2];
    char color_code[5];

    // Verificar a sintaxe do comando
    if (sscanf(request, "DBG %6s %d %1s %1s %1s %1s\n", plid, &playtime, c1, c2, c3, c4) != 6) {
        if (VERBOSE) {
            printf("Syntax error in command\n\n");
        }
        return "RDB ERR\n"; // Erro de sintaxe
    }
    snprintf(color_code, sizeof(color_code), "%s%s%s%s", c1, c2, c3, c4);
    
    if (!valid_colors(color_code) || strlen(request) != (23) || request[3] != ' '|| request[10] != ' '||
        request[14] != ' ' || request[16] != ' ' || request[18] != ' ' ||
        request[20] != ' ' || atoi(plid) < 100000 || playtime < 0 || playtime > 600) {
        if (VERBOSE) {
            printf("Syntax error in command\n\n");
        }
        return "RDB ERR\n"; // Erro de sintaxe
    }

    // Verificar se o jogador já tem um jogo ativo
    if (has_active_game(plid, FLAG_START)) {
        // Verificar se o tempo já passou. Se sim, terminar o jogo ativo
        time_t start_time, max_playtime, now;
        get_start_time(plid, &start_time);
        get_max_playtime(plid, &max_playtime);

        if (time(&now) > start_time + max_playtime) {
            if (VERBOSE) {
                printf("PLID=%s: TIMEOUT (game ended)\n", plid);
            }
            close_game(plid, max_playtime, 'T');
        } else { // Caso contrário, responder com NOK
            if (VERBOSE) {
                printf("PLID=%s: player already has an active game\n\n", plid);
            }
            return "RDB NOK\n"; // O jogador já tem um jogo em andamento
        }
    }

    // Associa o código secreto fornecido ao novo jogo
    char secret_code[CODE_SIZE + 1];
    snprintf(secret_code, sizeof(secret_code), "%c%c%c%c", c1[0], c2[0], c3[0], c4[0]);

    // Criar o ficheiro de jogo
    create_game(plid, secret_code, playtime, 'D');

    if (VERBOSE) {
        printf("New game (max %d sec); Colors: %c %c %c %c\n\n", playtime, secret_code[0], secret_code[1], secret_code[2], secret_code[3]);
    }

    // Responder com sucesso
    return "RDB OK\n";
}

const char* handle_quit(const char* request) {
    char plid[7];

    // Verificar a sintaxe do comando
    if (sscanf(request, "QUT %6s\n", plid) != 1) {
        if (VERBOSE) {
            printf("Syntax error in command\n\n");
        }
        return "RQT ERR\n"; // Erro de sintaxe
    }
    if(strlen(request) != 11 || atoi(plid) < 100000) {
        if (VERBOSE) {
            printf("Syntax error in command\n\n");
        }
        return "RQT ERR\n"; // Erro de sintaxe
    }

    // Verificar se o tempo já passou. Se sim, terminar o jogo ativo
    time_t start_time, max_playtime, now;

    // Verificar se o jogador tem um jogo ativo
    if (has_active_game(plid, FLAG_END)) {
        get_start_time(plid, &start_time);
        get_max_playtime(plid, &max_playtime);

        if (time(&now) > start_time + max_playtime) {
            if (VERBOSE) {
                printf("PLID=%s: TIMEOUT (game ended)\n", plid);
            }
            close_game(plid, max_playtime, 'T'); // Termina o jogo
        }
    }
    
    if (!has_active_game(plid, FLAG_END)) {
        if (VERBOSE) {
            printf("PLID=%s: player doesn't have an active game\n\n", plid);
        }
        return "RQT NOK\n";
    }

    // Obter o código secreto do jogo ativo
    char secret_code[CODE_SIZE + 1];
    if (!get_secret_code(plid, secret_code)) {
        return "RQT ERR\n"; // Erro ao obter informações do jogo
    }

    // Obter a hora de início do jogo ativo
    if (!get_start_time(plid, &start_time)) {
        return "RQT ERR\n"; // Erro ao obter a hora de início do jogo
    }

    // Calcula o tempo total de jogo
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
    if (VERBOSE) {
        printf("PLID=%s: QUIT (game ended)\n\n", plid);
    }
    return response;
}
