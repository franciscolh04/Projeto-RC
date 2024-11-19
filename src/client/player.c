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

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT + GN;
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];
    char *server_ip = "127.0.0.1";  // IP do servidor (localhost por padrão)
    int opt;
    socklen_t server_len = sizeof(server_addr);
    char protocol_choice;

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

    // Perguntar ao utilizador qual o protocolo utilizar
    printf("Escolha o protocolo (u para UDP, t para TCP): ");
    scanf(" %c", &protocol_choice);
    getchar();  // Limpar o buffer de entrada

    // Configurar o socket conforme o protocolo escolhido
    if (protocol_choice == 'u' || protocol_choice == 'U') {
        setup_udp_socket(&sockfd, &server_addr, server_ip, port);
        printf("Usando protocolo UDP\n");
    } else if (protocol_choice == 't' || protocol_choice == 'T') {
        setup_tcp_socket(&sockfd, &server_addr, server_ip, port);
        printf("Usando protocolo TCP\n");
    } else {
        fprintf(stderr, "Protocolo inválido. Use 'u' para UDP ou 't' para TCP.\n");
        exit(EXIT_FAILURE);
    }

    // Loop principal para interações contínuas
    while (1) {
        printf("Digite uma mensagem para enviar ao servidor (ou 'exit' para sair): ");
        fgets(buffer, BUF_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;  // Remover a nova linha do final da string

        // Se o comando for 'exit', sair do loop
        if (strcmp(buffer, "exit") == 0) {
            printf("Fechando o cliente...\n");
            break;
        }

        // Enviar a mensagem ao servidor
        if (protocol_choice == 'u' || protocol_choice == 'U') {
            if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("Erro ao enviar mensagem para o servidor (UDP)");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            printf("Mensagem enviada para o servidor (UDP): %s\n", buffer);
        } else if (protocol_choice == 't' || protocol_choice == 'T') {
            if (write(sockfd, buffer, strlen(buffer)) < 0) {
                perror("Erro ao enviar mensagem para o servidor (TCP)");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            printf("Mensagem enviada para o servidor (TCP): %s\n", buffer);
        }

        // Esperar pela resposta do servidor
        if (protocol_choice == 'u' || protocol_choice == 'U') {
            int n = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&server_addr, &server_len);
            if (n < 0) {
                perror("Erro ao receber resposta do servidor (UDP)");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            buffer[n] = '\0';  // Garantir que a resposta seja uma string válida
            printf("Resposta recebida do servidor (UDP): %s\n", buffer);
        } else if (protocol_choice == 't' || protocol_choice == 'T') {
            int n = read(sockfd, buffer, BUF_SIZE);
            if (n < 0) {
                perror("Erro ao receber resposta do servidor (TCP)");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            buffer[n] = '\0';
            printf("Resposta recebida do servidor (TCP): %s\n", buffer);
        }
    }

    // Fechar o socket após sair do loop
    close(sockfd);

    return 0;
}

// Função para configurar o socket UDP
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

// Função para configurar o socket TCP
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
