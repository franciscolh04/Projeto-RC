#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define DEFAULT_PORT "58000"
#define GN 0
#define BUF_SIZE 1024

void handle_udp_request(int sockfd, struct sockaddr_in *client_addr) {
    printf("vai ler UDP\n");
    char buffer[BUF_SIZE];
    socklen_t client_len = sizeof(*client_addr);
    int n = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)client_addr, &client_len);
    if (n < 0) {
        perror("Erro ao receber pacote UDP");
        return;
    }
    buffer[n] = '\0';
    printf("Recebido (UDP): %s\n", buffer);

    const char *response = "Mensagem recebida no servidor (UDP)";
    if (sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)client_addr, client_len) < 0) {
        perror("Erro ao enviar resposta UDP");
    }
}

void handle_tcp_request(int client_sockfd) {
    printf("vai ler TCP\n");
    char buffer[BUF_SIZE];
    int n = read(client_sockfd, buffer, BUF_SIZE - 1);

    if (n < 0) {
        perror("Erro ao ler do socket TCP");
        return;
    } else if (n == 0) {
        printf("Cliente fechou a conexão (TCP).\n");
        close(client_sockfd);
        return;
    }

    buffer[n] = '\0';
    printf("Recebido (TCP): %s\n", buffer);

    const char *response = "Mensagem recebida no servidor (TCP)";
    if (write(client_sockfd, response, strlen(response)) < 0) {
        perror("Erro ao enviar resposta TCP");
    }

    close(client_sockfd); // Fecha a conexão após responder
}

int main(int argc, char *argv[]) {
    int server_socket_udp, server_socket_tcp;
    int port_num = 58000 + GN;
    char port[6];
    snprintf(port, sizeof(port), "%d", port_num);

    struct addrinfo hints, *res_udp, *res_tcp;
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

    // Configuração do socket UDP
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &res_udp) != 0) {
        perror("Erro no getaddrinfo UDP");
        exit(1);
    }

    server_socket_udp = socket(res_udp->ai_family, res_udp->ai_socktype, res_udp->ai_protocol);
    if (server_socket_udp < 0) {
        perror("Erro ao criar o socket UDP");
        exit(1);
    }

    if (bind(server_socket_udp, res_udp->ai_addr, res_udp->ai_addrlen) < 0) {
        perror("Erro ao associar o socket UDP");
        exit(1);
    }

    // Configuração do socket TCP
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(NULL, port, &hints, &res_tcp) != 0) {
        perror("Erro no getaddrinfo TCP");
        exit(1);
    }

    server_socket_tcp = socket(res_tcp->ai_family, res_tcp->ai_socktype, res_tcp->ai_protocol);
    if (server_socket_tcp < 0) {
        perror("Erro ao criar o socket TCP");
        exit(1);
    }

    if (bind(server_socket_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen) < 0) {
        perror("Erro ao associar o socket TCP");
        exit(1);
    }

    if (listen(server_socket_tcp, 5) < 0) {
        perror("Erro ao colocar o socket TCP em modo de escuta");
        exit(1);
    }

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

        // Processar UDP
        if (FD_ISSET(server_socket_udp, &read_fds)) {
            struct sockaddr_in client_addr;
            handle_udp_request(server_socket_udp, &client_addr);
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

            // Adiciona cliente ao select
            handle_tcp_request(client_socket);
        }
    }

    freeaddrinfo(res_udp);
    freeaddrinfo(res_tcp);
    close(server_socket_udp);
    close(server_socket_tcp);
    return 0;
}
