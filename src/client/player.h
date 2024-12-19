#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "./../common/verifications.h"
#include "communication.h"

#define LOCALHOST "127.0.0.1"

// Variáveis globais
extern char PLID[7];
extern int MAX_PLAYTIME;
extern int NUM_TRIALS;
extern int EXIT;

// Funções para manipulação de comandos
void process_command(const char *input, char *formatted_command, char *protocol);
void interpret_server_response(const char *response);

// Função de tratamento de sinais
void handle_sigint(int sig);

#endif // PLAYER_H
