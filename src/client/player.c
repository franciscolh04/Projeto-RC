#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "player.h"

char PLID[7] = "";
int MAX_PLAYTIME = 0;
int NUM_TRIALS = 1;
int EXIT = 0;
int ONGOING_GAME = 0;

int main(int argc, char *argv[]) {
    // Configurar o manipulador de sinal SIGINT
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sa.sa_flags = 0; // Sem SA_RESTART
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Erro ao configurar sigaction");
        exit(EXIT_FAILURE);
    }


    int port = DEFAULT_PORT + GN;
    int sock_fd_udp;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];
    char formatted_command[BUF_SIZE];
    char response[BUF_SIZE];
    char protocol;
    char *server_ip = LOCALHOST;
    int opt;

    // Processar argumentos de linha de comando
    while ((opt = getopt(argc, argv, "n:p:")) != -1) {
        switch (opt) {
            case 'n':
                server_ip = optarg;  // Define o IP do servidor
                printf("IP do servidor: %s\n", server_ip);
                break;
            case 'p':
                port = atoi(optarg);  // Define a porta do servidor
                printf("Porta do servidor: %d\n", port);
                break;
            default:
                fprintf(stderr, "Uso: %s [-n GSIP] [-p GSport]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Configurar o socket UDP
    setup_udp_socket(&sock_fd_udp, &server_addr, server_ip, port);

    // Loop principal para interações contínuas
    while (!EXIT) {
        printf("Digite o comando a enviar para o servidor: ");
        if (fgets(buffer, BUF_SIZE, stdin) == NULL) {
            if (EXIT) break; // Verifica se SIGINT foi recebido durante o bloqueio de fgets
            perror("Erro ao ler input");
            continue;
        }
        buffer[strcspn(buffer, "\n")] = 0;  // Remover a nova linha do final da string

        // Processar o comando e escolher o protocolo
        process_command(buffer, formatted_command, &protocol);

        // Enviar a mensagem e receber a resposta conforme o protocolo
        int success = 0;
        if (protocol == 'U') {
            success = send_udp_message(sock_fd_udp, &server_addr, formatted_command, response);
        } else if (protocol == 'T') {
            success = send_tcp_message(&server_addr, server_ip, port, formatted_command, response);
        } else {
            printf("Comando inválido.\n");
            continue;
        }

        // Interpretar a resposta do servidor se a mensagem foi enviada com sucesso
        if (success) {
            interpret_server_response(response);
        }
    }

    // Fechar o socket UDP após sair do loop
    close(sock_fd_udp);

    return 0;
}

void handle_sigint(int sig) {
    (void)sig; // Evita o aviso de "unused parameter"
    EXIT = 1;
}

// Interpretar a resposta do servidor
void interpret_server_response(const char *response) {
    char status[BUF_SIZE], filename[BUF_SIZE];
    char c1[2], c2[2], c3[2], c4[2];
    char color_code[CODE_SIZE + 1];
    int filesize, nB, nW, num_trials_sv;

    // RSG
    if (sscanf(response, "RSG %s\n", status) == 1) {
        if (strcmp(response, "RSG OK\n") == 0) {
            printf("New game started (max %d sec)\n\n", MAX_PLAYTIME);
            NUM_TRIALS = 1;
            ONGOING_GAME = 1;
        } else if (strcmp(response, "RSG NOK\n") == 0) {
            printf("The player has an ongoing game\n\n");
        } else if (strcmp(response, "RSG ERR\n") == 0) {
            printf("Invalid arguments for specified command.\nUsage: start PLID (6-digit IST ID) max_playtime (up to 600 sec)\n\n");
        } else {
            printf("ERR\n\n");
        }
    // RTR
    } else if (sscanf(response, "RTR OK %d %d %d\n", &num_trials_sv, &nB, &nW) == 3) {
        if(strlen(response) != 13 || response[3] != ' ' || response[6] != ' ' || response[8] != ' ' ||
           response[10] != ' ' || nB > 4 || nW > 4 || nB < 0 || nW < 0 || num_trials_sv > 8 || num_trials_sv != NUM_TRIALS) {
            printf("ERR\n\n");
        } else if (nB == 4) {
            printf("WELL DONE! You guessed the key in %d trials\n\n", NUM_TRIALS);
            NUM_TRIALS = 1;
            ONGOING_GAME = 0;
        } else {
            printf("nB = %d, nW = %d\n\n", nB, nW);
            NUM_TRIALS++;
        }
    } else if (sscanf(response, "RTR ENT %1s %1s %1s %1s\n", c1, c2, c3, c4) == 4) {
        snprintf(color_code, sizeof(color_code), "%s%s%s%s", c1, c2, c3, c4);
        if (strlen(response) != 16 || response[3] != ' ' || response[7] != ' ' || response[9] != ' ' || response[11] != ' ' || response[13] != ' ' || !valid_colors(color_code)) {
            printf("ERR\n\n");
        }  else {
            printf("No more attempts available. The secret key was %s %s %s %s\n\n", c1, c2, c3, c4);
            NUM_TRIALS = 1;
            ONGOING_GAME = 0;
        }
    } else if (sscanf(response, "RTR ETM %1s %1s %1s %1s\n", c1, c2, c3, c4) == 4) {
        snprintf(color_code, sizeof(color_code), "%s%s%s%s", c1, c2, c3, c4);
        if (strlen(response) != 16 || response[3] != ' ' || response[7] != ' ' || response[9] != ' ' || response[11] != ' ' || response[13] != ' ' || !valid_colors(color_code)) {
            printf("ERR\n\n");
        } else {
            printf("Time is up. The secret key was %s %s %s %s\n\n", c1, c2, c3, c4);
            NUM_TRIALS = 1;
            ONGOING_GAME = 0;
        }
    } else if (sscanf(response, "RTR %s\n", status) == 1) {
        if (strcmp(response, "RTR DUP\n") == 0) {
            printf("Duplicate guess: The secret key you provided matches a previous trial\n\n");
        } else if (strcmp(response, "RTR INV\n") == 0) {
            printf("Invalid trial: The trial number is either not the expected value or does not match the previous guess for the current trial number\n\n");
        } else if (strcmp(response, "RTR NOK\n") == 0) {
            printf("To make a guess you have to start a game first. Use: start PLID max_playtime\n\n");
        } else if (strcmp(response, "RTR ERR\n") == 0) {
            printf("Invalid arguments for specified command.\n Usage: try c1 c2 c3 c4 (ci pertence {R, G, B, Y, O, P})\n\n");
        } else {
            printf("ERR\n\n");
        }
    // RQT
    } else if (sscanf(response, "RQT OK %1s %1s %1s %1s\n", c1, c2, c3, c4) == 4) {
        snprintf(color_code, sizeof(color_code), "%s%s%s%s", c1, c2, c3, c4);
        if (strlen(response) != 15 || response[3] != ' ' || response[6] != ' ' || response[8] != ' ' || response[10] != ' ' || response[12] != ' ' || !valid_colors(color_code)) {
            printf("ERR\n\n");
        } else {
            printf("The ongoing game has been terminated. The secret key was %s %s %s %s\n\n", c1, c2, c3, c4);
            ONGOING_GAME = 0;
        }
    } else if (sscanf(response, "RQT %s\n", status) == 1) {
        if (strcmp(response, "RQT NOK\n") == 0) {
            printf("No ongoing game: The player does not have an active game to quit\n\n");
        } else if (strcmp(response, "RQT ERR") == 0) {
            printf("The quit request could not be processed.\n\n");
        }
    // RDB
    } else if (sscanf(response, "RDB %s\n", status) == 1) {
        if (strcmp(response, "RDB OK\n") == 0) {
            printf("New game started (max %d sec)\n\n", MAX_PLAYTIME);
            NUM_TRIALS = 1;
        } else if (strcmp(response, "RDB NOK\n") == 0) {
            printf("The player has an ongoing game\n\n");
        } else if (strcmp(response, "RDB ERR\n") == 0) {
            printf("Invalid arguments for specified command.\nUsage: debug PLID (6-digit IST ID) max_playtime (up to 600 sec) c1 c2 c3 c4\n\n");
        } else {
            printf("ERRgay\n\n");
        }
    // RST
    } else if (sscanf(response, "RST %s %s %d", status, filename, &filesize) >= 1) {
        if(response[3] != ' ' || response[7] != ' ' || response[8 + strlen(filename)] != ' ' || response[strlen(response) - 1] != '\n') {
            printf("ERR\n\n");
        } else if (strcmp(status, "ACT") == 0 || strcmp(status, "FIN") == 0) {
            if (!save_file(response, filename, filesize)) {
                printf("Erro ao guardar o ficheiro '%s'.\n\n", filename);
            }
        } else if (strcmp(status, "NOK") == 0) {
            printf("Nenhum jogo ativo ou terminado encontrado para este jogador.\n\n");
        }
    // RSS
    } else if (sscanf(response, "RSS %s %s %d", status, filename, &filesize) >= 1) {
        if (strcmp(status, "EMPTY") == 0) {
            printf("Scoreboard vazia. Ainda não foi ganho nenhum jogo.\n\n");
        } else if (response[3] != ' ' || response[4 + strlen(status)] != ' ' || response[5 + strlen(status) + strlen(filename)] != ' ' || response[strlen(response) - 1] != '\n') {
            printf("ERR\n\n");
        } else if (strcmp(status, "OK") == 0) {
            if (!save_file(response, filename, filesize)) {
                printf("Erro ao guardar o ficheiro '%s'.\n\n", filename);
            }
        }
    // ERR
    } else {
        printf("ERR\n\n");
    }
}

void process_command(const char *input, char *formatted_command, char *protocol) {
    char c1[2], c2[2], c3[2], c4[2];
    char color_code[CODE_SIZE + 1];
    int num_dig = 0;
    char plid[7];
    strcpy(plid, PLID);

    // Análise do comando e definição do protocolo
    if (sscanf(input, "start %6s %d", PLID, &MAX_PLAYTIME) == 2) {
        if (ONGOING_GAME && strcmp(PLID, plid) != 0) {
            strcpy(PLID, plid);
            *protocol = 'X';  // X indica um comando inválido
            return;
        }
        num_dig = count_digits(MAX_PLAYTIME);
        if (strlen(input) != (13 + num_dig) || input[5] != ' ' || input[12] != ' ' || atoi(PLID) < 100000 || MAX_PLAYTIME < 0 || MAX_PLAYTIME > 600) {
            *protocol = 'X';  // X indica um comando inválido
        } else {
            snprintf(formatted_command, BUF_SIZE, "SNG %s %03d\n", PLID, MAX_PLAYTIME);
            *protocol = 'U';  // UDP para o comando "start"
        }
    } 
    // Adicionar outras condições conforme os outros comandos
    else if (sscanf(input, "try %1s %1s %1s %1s", c1, c2, c3, c4) == 4) {
        snprintf(color_code, sizeof(color_code), "%s%s%s%s", c1, c2, c3, c4);
        if (!valid_colors(color_code) || strlen(input) != 11 || input[3] != ' '|| input[5] != ' '||input[7] != ' ' || input[9] != ' ' || input[11] != '\0') {
            *protocol = 'X';  // X indica um comando inválido
        } else {
            snprintf(formatted_command, BUF_SIZE, "TRY %s %s %s %s %s %d\n", PLID, c1, c2, c3, c4, NUM_TRIALS);
            *protocol = 'U';  // UDP para o comando "try"
        }
    }
    else if (strcmp(input, "show_trials") == 0 || strcmp(input, "st") == 0) {
        snprintf(formatted_command, BUF_SIZE, "STR %s\n", PLID);
        *protocol = 'T';  // TCP para o comando "show_trials"
    }
    else if (strcmp(input, "scoreboard") == 0 || strcmp(input, "sb") == 0) {
        snprintf(formatted_command, BUF_SIZE, "SSB\n");
        *protocol = 'T';  // TCP para o comando "scoreboard"
    }
    else if (strcmp(input, "quit") == 0) {
        NUM_TRIALS = 1;
        MAX_PLAYTIME = 0;
        snprintf(formatted_command, BUF_SIZE, "QUT %s\n", PLID);
        *protocol = 'U';  // UDP para o comando "quit"
    }
    else if (strcmp(input, "exit") == 0) {
        NUM_TRIALS = 1;
        MAX_PLAYTIME = 0;
        snprintf(formatted_command, BUF_SIZE, "QUT %s\n", PLID);
        *protocol = 'U';  // UDP para o comando "exit"
        EXIT = 1;
    }
    else if (sscanf(input, "debug %6s %d %1s %1s %1s %1s", PLID, &MAX_PLAYTIME, c1, c2, c3, c4) == 6) {
        if (ONGOING_GAME) {
            strcpy(PLID, plid);
            *protocol = 'X';  // X indica um comando inválido
            return;
        }
        num_dig = count_digits(MAX_PLAYTIME);
        snprintf(color_code, sizeof(color_code), "%s%s%s%s", c1, c2, c3, c4);
        if (!valid_colors(color_code) || strlen(input) != (21 + num_dig) || input[5] != ' ' || input[12] != ' ' ||
            input[13 + num_dig] != ' ' || input[15 + num_dig] != ' ' || input[17 + num_dig] != ' ' || input[19 + num_dig] != ' ' ||
            input[21 + num_dig] != '\0' || atoi(PLID) < 100000 || MAX_PLAYTIME < 0 || MAX_PLAYTIME > 600) {
            *protocol = 'X';  // X indica um comando inválido
        } else {
            snprintf(formatted_command, BUF_SIZE, "DBG %s %d %s %s %s %s\n", PLID, MAX_PLAYTIME, c1, c2, c3, c4);
            *protocol = 'U';  // UDP para o comando "debug"
        }
    }
    else {
        *protocol = 'X';  // X indica um comando não reconhecido
    }
}


