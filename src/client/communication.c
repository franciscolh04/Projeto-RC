#include "communication.h"

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

// Enviar mensagem UDP (com reenvio após 3 timeouts e terminar após 5 timeouts) e retornar a resposta
int send_udp_message(int sockfd, struct sockaddr_in *server_addr, char *message, char *response) {
    const int max_retries = 5; // Número máximo total de timeouts antes de terminar
    const int resend_threshold = 3; // Número de timeouts antes de reenviar a mensagem
    const int timeout_seconds = 2; // Tempo limite em segundos para cada tentativa

    int total_timeouts = 0; // Contador de timeouts cumulativos
    int retries = 0; // Contador de reenvios da mensagem

    while (total_timeouts < max_retries) {
        if (total_timeouts == 0 || total_timeouts % resend_threshold == 0) {
            // Reenvia a mensagem após cada resend_threshold timeouts
            if (total_timeouts != 0) {
                printf("Reenviar mensagem (tentativa %d)...\n", retries + 1);
            }
            if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
                perror("Erro ao enviar mensagem para o servidor (UDP)");
                return 0;
            }
            retries++;
        }

        // Configurar timeout para receber resposta
        struct timeval timeout;
        timeout.tv_sec = timeout_seconds;
        timeout.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("Erro ao configurar timeout do socket UDP");
            return 0;
        }

        // Esperar pela resposta do servidor
        socklen_t server_len = sizeof(*server_addr);
        int n = recvfrom(sockfd, response, BUF_SIZE, 0, (struct sockaddr *)server_addr, &server_len);
        if (n >= 0) {
            response[n] = '\0'; // Garante que a mensagem é uma string válida
            return 1; // Resposta recebida com sucesso
        } else if (n < 0) {
            perror("Timeout ou erro ao receber resposta do servidor (UDP)");
            total_timeouts++;
            printf("Timeout %d/%d\n", total_timeouts, max_retries);
        }
    }

    // Se todas as tentativas falharem, termina a aplicação
    printf("Não foi possível obter resposta do servidor após %d timeouts. Terminando a aplicação.\n", max_retries);
    EXIT = 1; // Terminar a aplicação
    return 0;
}

// Estabelecer conexão TCP, enviar mensagem, receber resposta e fechar conexão e retornar a resposta
int send_tcp_message(struct sockaddr_in *server_addr, char *server_ip, int port, char *message, char *response) {
    int sockfd;

    // Configura e conecta o socket TCP
    setup_tcp_socket(&sockfd, server_addr, server_ip, port);

    // Envia a mensagem para o servidor
    int total_written = 0, total_read = 0, n;
    int bytes_to_write = strlen(message);

    while (total_written < bytes_to_write) {
        n = write(sockfd, message + total_written, bytes_to_write - total_written);
        if (n < 0) {
            perror("Erro ao enviar mensagem para o servidor (TCP)");
            close(sockfd);
            return 0;
        }
        total_written += n;
    }

    // Lê a resposta inicial do servidor
    while (total_read < BUF_SIZE - 1) {
        n = read(sockfd, response + total_read, BUF_SIZE - 1 - total_read);
        if (n < 0) {
            perror("Erro ao receber resposta do servidor (TCP)");
            close(sockfd);
            return 0;
        }
        if (n == 0) { // Conexão fechada pelo servidor
            break;
        }
            total_read += n;
    }

    response[total_read] = '\0'; // Garante que a mensagem é uma string válida

    // Fecha a conexão TCP após processar a resposta
    close(sockfd);
    return 1;
}


int save_file(const char *response, const char *filename, int filesize) {
    //printf("A processar ficheiro '%s' (%d bytes)...\n", filename, filesize);

    // Abre o ficheiro local para escrita
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Erro ao abrir ficheiro para escrita");
        return 0;
    }

    // Encontrar o início dos dados Fdata no buffer response
    const char *file_data = response;

    // Saltar o cabeçalho "RST status Fname Fsize"
    
    file_data = strchr(file_data, ' '); // Salta "RST"
    if (!file_data) { fclose(file); return 0; }

    for(int i = 0; i < 3; i++ ) {
        file_data = strchr(file_data + 1, ' '); // Salta "status"
        if (!file_data) { fclose(file); return 0; }
    }    

    // Avançar para além do espaço para chegar ao início de Fdata
    file_data++;

    // Escrever os dados no ficheiro
    int bytes_written = 0;
    while (bytes_written < filesize) {
        int chunk_size = fwrite(file_data + bytes_written, 1, filesize - bytes_written, file);
        if (chunk_size <= 0) {
            perror("Erro ao escrever ficheiro");
            fclose(file);
            return 0;
        }
        bytes_written += chunk_size;
    }

    fclose(file);
    printf("Ficheiro '%s' recebido e guardado com sucesso!:\n", filename);
    printf("----------------------------------------------------------------------\n");
    printf("%s\n", file_data);

    return 1;
}