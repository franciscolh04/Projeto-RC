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

void handle_udp_request(int sockfd, struct sockaddr_in *client_addr);
void handle_tcp_request(int sockfd, int client_sockfd, struct sockaddr_in *client_addr);
void print_verbose(int verbose, struct sockaddr_in *client_addr, const char *request_type);

int main(int argc, char *argv[]) {
    int server_socket_udp, server_socket_tcp;
    struct sockaddr_in server_addr_udp, server_addr_tcp;
    int port = DEFAULT_PORT + GN;
    int verbose = 0;
    char buffer[BUF_SIZE];
    int opt;

    // Processamento dos argumentos da linha de comando
    while ((opt = getopt(argc, argv, "p:v")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                fprintf(stderr, "Uso: %s [-p GSport] [-v]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Criação do socket UDP
    server_socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket_udp < 0) {
        perror("Erro ao criar o socket UDP");
        exit(1);
    }

    // Criação do socket TCP
    server_socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_tcp < 0) {
        perror("Erro ao criar o socket TCP");
        exit(1);
    }

    // Configuração do endereço do servidor
    memset(&server_addr_udp, 0, sizeof(server_addr_udp));
    server_addr_udp.sin_family = AF_INET;
    server_addr_udp.sin_addr.s_addr = INADDR_ANY;
    server_addr_udp.sin_port = htons(port);

    memset(&server_addr_tcp, 0, sizeof(server_addr_tcp));
    server_addr_tcp.sin_family = AF_INET;
    server_addr_tcp.sin_addr.s_addr = INADDR_ANY;
    server_addr_tcp.sin_port = htons(port);

    // Associação dos sockets ao porto
    if (bind(server_socket_udp, (struct sockaddr *)&server_addr_udp, sizeof(server_addr_udp)) < 0) {
        perror("Erro ao associar o socket UDP");
        exit(1);
    }
    if (bind(server_socket_tcp, (struct sockaddr *)&server_addr_tcp, sizeof(server_addr_tcp)) < 0) {
        perror("Erro ao associar o socket TCP");
        exit(1);
    }

    // Preparação do socket TCP para escutar
    listen(server_socket_tcp, 5);

    printf("Servidor a escutar na porta %d...\n", port);

    // Escuta as requisições
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_socket_udp, &read_fds);
        FD_SET(server_socket_tcp, &read_fds);

        // Usa select para esperar por atividade nos sockets
        if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Erro no select");
            exit(1);
        }

        // Processa requisição UDP
        if (FD_ISSET(server_socket_udp, &read_fds)) {
            struct sockaddr_in client_addr;
            handle_udp_request(server_socket_udp, &client_addr);
        }

        // Processa requisição TCP
        if (FD_ISSET(server_socket_tcp, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server_socket_tcp, (struct sockaddr *)&client_addr, &client_len);
            if (client_socket < 0) {
                perror("Erro ao aceitar conexão TCP");
                continue;
            }
            handle_tcp_request(server_socket_tcp, client_socket, &client_addr);
        }
    }

    close(server_socket_udp);
    close(server_socket_tcp);
    return 0;
}

void handle_udp_request(int sockfd, struct sockaddr_in *client_addr) {
    char buffer[BUF_SIZE];
    socklen_t client_len = sizeof(*client_addr);
    int n = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)client_addr, &client_len);
    if (n < 0) {
        perror("Erro ao receber pacote UDP");
        return;
    }
    buffer[n] = '\0';
    printf("Recebido (UDP): %s\n", buffer);

    // Envia resposta ao cliente UDP
    const char *response = "Mensagem recebida no servidor (UDP)";
    if (sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)client_addr, client_len) < 0) {
        perror("Erro ao enviar resposta UDP");
    }
}

void handle_tcp_request(int sockfd, int client_sockfd, struct sockaddr_in *client_addr) {
    char buffer[BUF_SIZE];
    int n = read(client_sockfd, buffer, BUF_SIZE - 1);
    if (n < 0) {
        perror("Erro ao ler do socket TCP");
        return;
    }
    buffer[n] = '\0';
    printf("Recebido (TCP): %s\n", buffer);

    // Envia resposta ao cliente TCP
    const char *response = "Mensagem recebida no servidor (TCP)";
    if (write(client_sockfd, response, strlen(response)) < 0) {
        perror("Erro ao enviar resposta TCP");
    }

    close(client_sockfd);  // Fecha a conexão após responder
}

void print_verbose(int verbose, struct sockaddr_in *client_addr, const char *request_type) {
    if (verbose) {
        printf("Requisição recebida de %s:%d, tipo: %s\n",
               inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port), request_type);
    }
}
