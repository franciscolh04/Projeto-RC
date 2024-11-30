#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include "state.h"
#include "command_handler.h"


#define DEFAULT_PORT "58000"
#define GN 0
#define BUF_SIZE 1024

// Funções para configurar os sockets e lidar com os pedidos
int setup_udp_socket(const char *port);
int setup_tcp_socket(const char *port);
void handle_udp_request(int sockfd, struct sockaddr_in client_addr);
void handle_tcp_request(int client_sockfd);

// Função para interpretar o pedido do cliente
const char* interpret_player_request(const char *message);


// Variáveis globais para armazenar as listas ligadas de jogadores e jogos
Player *players_head = NULL;  // Cabeça da lista ligada de jogadores
Game *games_head = NULL;      // Cabeça da lista ligada de jogos


int main(int argc, char *argv[]) {
    int server_socket_udp, server_socket_tcp;
    int port_num = 58000 + GN;
    char port[6];
    snprintf(port, sizeof(port), "%d", port_num);

    int opt, verbose = 0;

    // Processar argumentos
    while ((opt = getopt(argc, argv, "p:v")) != -1) {
        switch (opt) {
            case 'p':
                snprintf(port, sizeof(port), "%s", optarg);
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                fprintf(stderr, "Uso: %s [-p GSport] [-v]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Configuração do socket UDP e TCP
    server_socket_udp = setup_udp_socket(port);
    server_socket_tcp = setup_tcp_socket(port);

    printf("Servidor a escutar na porta %s...\n", port);

    // Loop principal
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_socket_udp, &read_fds);
        FD_SET(server_socket_tcp, &read_fds);

        int max_fd = (server_socket_udp > server_socket_tcp) ? server_socket_udp : server_socket_tcp;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Erro no select");
            exit(1);
        }

        // Processar UDP (no processo pai)
        if (FD_ISSET(server_socket_udp, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            handle_udp_request(server_socket_udp, client_addr);
        }

        // Processar TCP
        if (FD_ISSET(server_socket_tcp, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server_socket_tcp, (struct sockaddr *)&client_addr, &client_len);
            if (client_socket < 0) {
                perror("Erro ao aceitar conexão TCP");
                continue;
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("Erro ao criar processo filho");
                close(client_socket);
            } else if (pid == 0) {
                // Processo filho lida com este cliente TCP
                close(server_socket_tcp); // O filho não precisa do socket de escuta
                handle_tcp_request(client_socket);
                exit(0); // Termina o processo filho
            } else {
                // Processo pai
                close(client_socket); // O pai não precisa do socket cliente
            }
        }

        // Evitar processos zumbis
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }

    close(server_socket_udp);
    close(server_socket_tcp);

    return 0;
}

// Função para configurar o socket UDP
int setup_udp_socket(const char *port) {
    struct addrinfo hints, *res_udp;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &res_udp) != 0) {
        perror("Erro no getaddrinfo UDP");
        exit(1);
    }

    sockfd = socket(res_udp->ai_family, res_udp->ai_socktype, res_udp->ai_protocol);
    if (sockfd < 0) {
        perror("Erro ao criar o socket UDP");
        exit(1);
    }

    if (bind(sockfd, res_udp->ai_addr, res_udp->ai_addrlen) < 0) {
        perror("Erro ao associar o socket UDP");
        exit(1);
    }

    freeaddrinfo(res_udp);
    return sockfd;
}

// Função para configurar o socket TCP
int setup_tcp_socket(const char *port) {
    struct addrinfo hints, *res_tcp;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &res_tcp) != 0) {
        perror("Erro no getaddrinfo TCP");
        exit(1);
    }

    sockfd = socket(res_tcp->ai_family, res_tcp->ai_socktype, res_tcp->ai_protocol);
    if (sockfd < 0) {
        perror("Erro ao criar o socket TCP");
        exit(1);
    }

    if (bind(sockfd, res_tcp->ai_addr, res_tcp->ai_addrlen) < 0) {
        perror("Erro ao associar o socket TCP");
        exit(1);
    }

    if (listen(sockfd, 5) < 0) {
        perror("Erro ao colocar o socket TCP em modo de escuta");
        exit(1);
    }

    freeaddrinfo(res_tcp);
    return sockfd;
}

// Função para lidar com requisições UDP no processo pai
void handle_udp_request(int sockfd, struct sockaddr_in client_addr) {
    char buffer[BUF_SIZE];
    socklen_t client_len = sizeof(client_addr);

    printf("Processo pai a tratar cliente UDP %s:%d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Recebe mensagem do cliente
    int n = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
    if (n < 0) {
        perror("Erro ao receber pacote UDP");
        return;
    }

    buffer[n] = '\0'; // Termina a string
    printf("Recebido do cliente UDP: %s\n", buffer);

    // Chama a função para interpretar a mensagem
    const char *response = interpret_player_request(buffer);

    // Envia resposta ao cliente
    if (sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)&client_addr, client_len) < 0) {
        perror("Erro ao enviar resposta UDP");
    }
}


void handle_tcp_request(int client_sockfd) {
    char buffer[BUF_SIZE];
    int total_read = 0, total_written = 0, n;

    // Lê os dados do cliente em um loop
    while (1) {
        n = read(client_sockfd, buffer + total_read, BUF_SIZE - 1 - total_read);
        if (n < 0) {
            perror("Erro ao ler do socket TCP");
            close(client_sockfd);
            return;
        }
        if (n == 0) { // Conexão fechada pelo cliente
            printf("[TCP] Cliente fechou a conexão.\n");
            break;
        }

        total_read += n;

        // Verifica se a mensagem está completa (terminada com '\n' ou terminador)
        if (buffer[total_read - 1] == '\n' || total_read >= BUF_SIZE - 1) {
            break;
        }
    }

    buffer[total_read] = '\0'; // Garante que a string lida está terminada corretamente
    printf("Recebido do cliente TCP: %s\n", buffer);

    // Chama a função para interpretar a mensagem
    const char *response = interpret_player_request(buffer);

    // Envia a resposta ao cliente
    while (total_written < strlen(response)) {
        n = write(client_sockfd, response + total_written, strlen(response) - total_written);
        if (n < 0) {
            perror("Erro ao enviar resposta TCP");
            close(client_sockfd);
            return;
        }
        total_written += n;
    }

    // Fecha a conexão TCP após responder
    close(client_sockfd);
}

// Função para interpretar a mensagem do cliente
const char* interpret_player_request(const char *message) {
    char command[4];  // String para armazenar o comando extraído (3 caracteres + 1 para '\0')
    
    // Extrai os primeiros 3 caracteres da mensagem
    if (sscanf(message, "%3s", command) != 1) {
        return "ERR Comando inválido\n";
    }

    // Compara o comando extraído
    if (strcmp(command, "SNG") == 0) {
        return handle_start(message);
    } else if (strcmp(command, "TRY") == 0) {
        return handle_try();
    } else if (strcmp(command, "STR") == 0) {
        return handle_show_trials();
    } else if (strcmp(command, "SSB") == 0) {
        return handle_scoreboard();
    } else if (strcmp(command, "DBG") == 0) {
        return handle_debug();
    } else if (strcmp(command, "QUT") == 0) {
        return handle_quit();
    } else {
        return "ERR Comando desconhecido\n";
    }
}
