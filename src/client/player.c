#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 58000
#define GN 0
#define BUF_SIZE 1024

int PLID = -1;
int MAX_PLAYTIME = 0;
int NUM_TRIALS = 0;

// Funções para configurar os sockets
void setup_udp_socket(int *sockfd, struct sockaddr_in *server_addr, char *server_ip, int port);
void setup_tcp_socket(int *sockfd, struct sockaddr_in *server_addr, char *server_ip, int port);
int send_udp_message(int sockfd, struct sockaddr_in *server_addr, char *message, char *response);
int send_tcp_message(struct sockaddr_in *server_addr, char *server_ip, int port, char *message, char *response);

// Função para interpretar a resposta do servidor
void interpret_server_response(const char *response);

// Função para processar o comando do utilizador
void process_command(const char *input, char *formatted_command, char *protocol);

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT + GN;
    int sock_fd_udp;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];
    char formatted_command[BUF_SIZE];
    char response[BUF_SIZE];
    char protocol;
    char *server_ip = "127.0.0.1";  // IP do servidor (localhost por padrão)
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
    while (1) {
        printf("Digite um comando para enviar ao servidor (ou 'exit' para sair): ");
        fgets(buffer, BUF_SIZE, stdin);
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
            printf("Comando inválido ou protocolo não suportado.\n");
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

// Configuração do socket UDP
void setup_udp_socket(int *sockfd, struct sockaddr_in *server_addr, char *server_ip, int port) {
    *sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (*sockfd < 0) {
        perror("Erro ao criar socket UDP");
        exit(EXIT_FAILURE);
    }

    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr->sin_addr) <= 0) {
        perror("Erro ao configurar o endereço IP do servidor (UDP)");
        exit(EXIT_FAILURE);
    }
}

// Configuração de socket TCP
void setup_tcp_socket(int *sockfd, struct sockaddr_in *server_addr, char *server_ip, int port) {
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd < 0) {
        perror("Erro ao criar socket TCP");
        exit(EXIT_FAILURE);
    }

    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr->sin_addr) <= 0) {
        perror("Erro ao configurar o endereço IP do servidor (TCP)");
        close(*sockfd);
        exit(EXIT_FAILURE);
    }

    if (connect(*sockfd, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("Erro ao conectar ao servidor (TCP)");
        close(*sockfd);
        exit(EXIT_FAILURE);
    }
}

// Enviar mensagem UDP e retornar a resposta
int send_udp_message(int sockfd, struct sockaddr_in *server_addr, char *message, char *response) {
    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("Erro ao enviar mensagem para o servidor (UDP)");
        return 0;
    }

    socklen_t server_len = sizeof(*server_addr);
    int n = recvfrom(sockfd, response, BUF_SIZE, 0, (struct sockaddr *)server_addr, &server_len);
    if (n < 0) {
        perror("Erro ao receber resposta do servidor (UDP)");
        return 0;
    }

    response[n] = '\0'; // Garantir que a mensagem é uma string válida
    return 1;
}

// Estabelecer conexão TCP, enviar mensagem, receber resposta e fechar conexão e retornar a resposta
int send_tcp_message(struct sockaddr_in *server_addr, char *server_ip, int port, char *message, char *response) {
    int sockfd;

    // Configura e conecta o socket TCP
    setup_tcp_socket(&sockfd, server_addr, server_ip, port);

    // Envia a mensagem para o servidor
    if (write(sockfd, message, strlen(message)) < 0) {
        perror("Erro ao enviar mensagem para o servidor (TCP)");
        close(sockfd);
        return 0;
    }

    int n = read(sockfd, response, BUF_SIZE);
    if (n < 0) {
        perror("Erro ao receber resposta do servidor (TCP)");
        close(sockfd);
        return 0;
    }

    response[n] = '\0'; // Garantir que a mensagem é uma string válida
    close(sockfd);
    return 1;
}

// Interpretar a resposta do servidor
void interpret_server_response(const char *response) {
    //printf("Resposta do servidor: %s\n", response);
    char command[BUF_SIZE];
    char status[BUF_SIZE];

    // Extrai o comando e o status da resposta
    /*
    if (sscanf(response, "%s %s", command, status) < 2) {
        printf("Resposta do servidor mal formatada: %s\n", response);
        return;
    }
    */

    // Lida com diferentes comandos recebidos do servidor
    if (sscanf(response, "RSG %s\n", status) == 1) {
        if (strcmp(status, "OK") == 0) {
            printf("New game started (max %d sec)\n", MAX_PLAYTIME);
        } else if (strcmp(status, "NOK") == 0) {
            printf("The player has an ongoing game.\n");
        } else if (strcmp(status, "ERR") == 0) {
            printf("Invalid arguments for specified command.\nUsage: start PLID (6-digit IST ID) max_playtime (up to 600 sec)\n");
        }
    } else if (sscanf(command, "RTR %s\n", status) == 1) {
        // Adicionar tratamento para o comando "RTR"
    }
}


void process_command(const char *input, char *formatted_command, char *protocol) {
    char command[BUF_SIZE];
    int max_playtime;
    char c1[2], c2[2], c3[2], c4[2];

    // Análise do comando e definição do protocolo
    if (sscanf(input, "start %d %d", &PLID, &max_playtime) == 2) {
        snprintf(formatted_command, BUF_SIZE, "SNG %d %d\n", PLID, max_playtime);
        *protocol = 'U';  // UDP para o comando "start"
    } 
    // Adicionar outras condições conforme os outros comandos
    else if (sscanf(input, "try %1s %1s %1s %1s", c1, c2, c3, c4) == 4) {
        NUM_TRIALS++;
        snprintf(formatted_command, BUF_SIZE, "TRY %d %s %s %s %s %d\n", PLID, c1, c2, c3, c4, NUM_TRIALS);
        *protocol = 'U';  // UDP para o comando "try"
    }
    else if (strcmp(input, "show_trials") == 0 || strcmp(input, "st") == 0) {
        snprintf(formatted_command, BUF_SIZE, "STR %d\n", PLID);
        *protocol = 'T';  // TCP para o comando "show_trials"
    }
    else if (strcmp(input, "scoreboard") == 0 || strcmp(input, "sb") == 0) {
        snprintf(formatted_command, BUF_SIZE, "SSB\n");
        *protocol = 'T';  // TCP para o comando "scoreboard"
    }
    else if (strcmp(input, "quit") == 0) {
        NUM_TRIALS = 0;
        MAX_PLAYTIME = 0;
        snprintf(formatted_command, BUF_SIZE, "QUT %d\n", PLID);
        *protocol = 'U';  // UDP para o comando "quit"
    }
    else if (strcmp(input, "exit") == 0) {
        NUM_TRIALS = 0;
        MAX_PLAYTIME = 0;
        snprintf(formatted_command, BUF_SIZE, "QUT %d\n", PLID);
        *protocol = 'U';  // UDP para o comando "exit"
    }
    else if (sscanf(input, "debug %d %d %1s %1s %1s %1s", &PLID, &max_playtime, c1, c2, c3, c4) == 6) {
        snprintf(formatted_command, BUF_SIZE, "DBG %d %d %s %s %s %s\n", PLID, max_playtime, c1, c2, c3, c4);
        *protocol = 'U';  // UDP para o comando "debug"
    }
    else {
        *protocol = 'X';  // X indica um comando não reconhecido
    }
}
