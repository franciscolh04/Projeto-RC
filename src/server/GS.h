// GS.h
#ifndef GS_H
#define GS_H

#include <netinet/in.h>
#include "./../common/verifications.h"
#include "state.h"
#include "command_handler.h"

extern int VERBOSE; // Declaração da variável global

// Funções para configurar os sockets e lidar com os pedidos
int setup_udp_socket(const char *port);
int setup_tcp_socket(const char *port);

// Funções para lidar com pedidos UDP e TCP
void handle_udp_request(int sockfd, struct sockaddr_in client_addr);
void handle_tcp_request(int client_sockfd, struct sockaddr_in client_addr);

// Função para interpretar o pedido do cliente
const char* interpret_player_request(const char *message);

#endif
