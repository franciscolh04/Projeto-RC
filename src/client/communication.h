#ifndef COMMUNICATION_H
#define COMMUNICATION_H

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
#include "./../common/verifications.h"

// Funções para configurar os sockets
void setup_udp_socket(int *sockfd, struct sockaddr_in *server_addr, char *server_ip, int port);
void setup_tcp_socket(int *sockfd, struct sockaddr_in *server_addr, char *server_ip, int port);

// Funções para enviar mensagens e processar respostas
int send_udp_message(int sockfd, struct sockaddr_in *server_addr, char *message, char *response);
int send_tcp_message(struct sockaddr_in *server_addr, char *server_ip, int port, char *message, char *response);

// Função para salvar ficheiros recebidos
int save_file(const char *response, const char *filename, int filesize);

#endif // COMMUNICATION_H