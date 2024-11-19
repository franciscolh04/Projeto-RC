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

// Funções para configurar os sockets
void setup_udp_socket(int *sockfd, struct sockaddr_in *server_addr, char *server_ip, int port);
void setup_tcp_socket(int *sockfd, struct sockaddr_in *server_addr, char *server_ip, int port);
void send_udp_message(int sockfd, struct sockaddr_in *server_addr, char *message);
void send_tcp_message(int sockfd, char *message);
void receive_udp_response(int sockfd, struct sockaddr_in *server_addr);
void receive_tcp_response(int sockfd);

// Função para processar o comando do utilizador
void process_command(const char *input, char *formatted_command, char *protocol);

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT + GN;
    int sock_fd_udp, sock_fd_tcp;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];
    char formatted_command[BUF_SIZE];
    char protocol;
    char *server_ip = "127.0.0.1";  // IP do servidor (localhost por padrão)
    int opt;

    // Processar argumentos de linha de comando
    while ((opt = getopt(argc, argv, "n:p:")) != -1) {
        switch (opt) {
            case 'n':
                server_ip = optarg;  // Define o IP do servidor
                break;
            case 'p':
                port = atoi(optarg);  // Define a porta do servidor
                break;
            default:
                fprintf(stderr, "Uso: %s [-n GSIP] [-p GSport]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Configurar os sockets UDP e TCP
    setup_udp_socket(&sock_fd_udp, &server_addr, server_ip, port);
    setup_tcp_socket(&sock_fd_tcp, &server_addr, server_ip, port);

    // Loop principal para interações contínuas
    while (1) {
        printf("Digite um comando para enviar ao servidor (ou 'exit' para sair): ");
        fgets(buffer, BUF_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;  // Remover a nova linha do final da string

        // Se o comando for 'exit', sair do loop
        if (strcmp(buffer, "exit") == 0) {
            printf("Fechando o cliente...\n");
            break;
        }

        // Processar o comando e escolher o protocolo
        process_command(buffer, formatted_command, &protocol);

        // Enviar a mensagem e receber a resposta conforme o protocolo
        if (protocol == 'U') {
            send_udp_message(sock_fd_udp, &server_addr, formatted_command);
            receive_udp_response(sock_fd_udp, &server_addr);
        } else if (protocol == 'T') {
            send_tcp_message(sock_fd_tcp, formatted_command);
            receive_tcp_response(sock_fd_tcp);
        } else {
            printf("Comando inválido ou protocolo não suportado.\n");
        }
    }

    // Fechar os sockets após sair do loop
    close(sock_fd_udp);
    close(sock_fd_tcp);

    return 0;
}

// Implementação das funções auxiliares

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
        exit(EXIT_FAILURE);
    }

    if (connect(*sockfd, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("Erro ao conectar ao servidor (TCP)");
        close(*sockfd);
        exit(EXIT_FAILURE);
    }
}

void send_udp_message(int sockfd, struct sockaddr_in *server_addr, char *message) {
    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("Erro ao enviar mensagem para o servidor (UDP)");
    }
}

void send_tcp_message(int sockfd, char *message) {
    if (write(sockfd, message, strlen(message)) < 0) {
        perror("Erro ao enviar mensagem para o servidor (TCP)");
    }
}

void receive_udp_response(int sockfd, struct sockaddr_in *server_addr) {
    char buffer[BUF_SIZE];
    socklen_t server_len = sizeof(*server_addr);
    int n = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)server_addr, &server_len);
    if (n < 0) {
        perror("Erro ao receber resposta do servidor (UDP)");
    } else {
        buffer[n] = '\0';
        printf("Resposta recebida do servidor (UDP): %s\n", buffer);
    }
}

void receive_tcp_response(int sockfd) {
    char buffer[BUF_SIZE];
    int n = read(sockfd, buffer, BUF_SIZE);
    if (n < 0) {
        perror("Erro ao receber resposta do servidor (TCP)");
    } else {
        buffer[n] = '\0';
        printf("Resposta recebida do servidor (TCP): %s\n", buffer);
    }
}

void process_command(const char *input, char *formatted_command, char *protocol) {
    char command[BUF_SIZE];
    int plid, max_playtime;

    // Análise do comando e definição do protocolo
    if (sscanf(input, "start %d %d", &plid, &max_playtime) == 2) {
        snprintf(formatted_command, BUF_SIZE, "SNG %d %d", plid, max_playtime);
        *protocol = 'U';  // UDP para o comando "start"
    } 
    // Adicionar outras condições conforme os outros comandos
    else {
        *protocol = 'X';  // X indica um comando não reconhecido
    }
}
