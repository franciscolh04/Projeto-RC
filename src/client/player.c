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
int NUM_TRIALS = 1;
int EXIT = 0;

// Funções para configurar os sockets e enviar mensagens
void setup_udp_socket(int *sockfd, struct sockaddr_in *server_addr, char *server_ip, int port);
void setup_tcp_socket(int *sockfd, struct sockaddr_in *server_addr, char *server_ip, int port);
int send_udp_message(int sockfd, struct sockaddr_in *server_addr, char *message, char *response);
int send_tcp_message(struct sockaddr_in *server_addr, char *server_ip, int port, char *message, char *response);

// Função para processar o comando do utilizador
void process_command(const char *input, char *formatted_command, char *protocol);

// Função para interpretar a resposta do servidor
void interpret_server_response(const char *response);


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
    while (!EXIT) {
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


int save_file(const char *response, const char *filename, int filesize) {
    printf("A processar ficheiro '%s' (%d bytes)...\n", filename, filesize);

    // Abre o ficheiro local para escrita
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Erro ao abrir ficheiro para escrita");
        return 0;
    }

    // Encontra o início dos dados do ficheiro no buffer
    const char *file_data = strchr(response, '\n'); // Localiza o primeiro '\n' (fim da linha do cabeçalho)
    if (!file_data) {
        printf("Erro: Cabeçalho mal formatado na resposta.\n");
        fclose(file);
        return 0;
    }
    file_data += 1; // Avança para depois do '\n'

    // Calcula o comprimento dos dados (tamanho da resposta - posição após o cabeçalho)
    int data_len = strlen(file_data); // Calcula o comprimento da string a partir da posição atual dos dados

    if (data_len < filesize) {
        printf("Erro: Dados do ficheiro incompletos na resposta (%d/%d bytes).\n", data_len, filesize);
        fclose(file);
        return 0;
    }

    // Escreve os dados no ficheiro em loop
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
    printf("Ficheiro '%s' recebido e armazenado com sucesso!\n", filename);

    return 1;
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

    response[n] = '\0'; // Garante que a mensagem é uma string válida
    return 1;
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


// Interpretar a resposta do servidor
void interpret_server_response(const char *response) {
    printf("Resposta do servidor: %s\n", response);
    char command[BUF_SIZE], status[BUF_SIZE], filename[BUF_SIZE];
    char c1[2], c2[2], c3[2], c4[2];
    int filesize, nB, nW;

    // RSG
    if (sscanf(response, "RSG %s\n", status) == 1) {
        if (strcmp(status, "OK") == 0) {
            printf("New game started (max %d sec)\n", MAX_PLAYTIME);
        } else if (strcmp(status, "NOK") == 0) {
            printf("The player has an ongoing game\n");
        } else if (strcmp(status, "ERR") == 0) {
            printf("Invalid arguments for specified command.\nUsage: start PLID (6-digit IST ID) max_playtime (up to 600 sec)\n");
        }
    // RTR
    } else if (sscanf(response, "RTR OK %d %d %d\n", &NUM_TRIALS, &nB, &nW) == 3) {
        if (nB == 4) {
            printf("WELL DONE! You guessed the key in %d trials\n", NUM_TRIALS);
            NUM_TRIALS = 1;
        } else {
            printf("nB = %d, nW = %d\n", nB, nW);
            NUM_TRIALS++;
        }
    } else if (sscanf(response, "RTR ENT %1s %1s %1s %1s\n", c1, c2, c3, c4) == 4) {
        printf("No more attempts available. The secret key was %s %s %s %s\n", c1, c2, c3, c4);
        NUM_TRIALS = 1;
    } else if (sscanf(response, "RTR ETM %1s %1s %1s %1s\n", c1, c2, c3, c4) == 4) {
        printf("Time is up. The secret key was %s %s %s %s\n", c1, c2, c3, c4);
        NUM_TRIALS = 1;
    } else if (sscanf(response, "RTR %s\n", status) == 1) {
        if (strcmp(status, "DUP") == 0) {
            printf("Duplicate guess: The secret key you provided matches a previous trial\n");
        } else if (strcmp(status, "INV") == 0) {
            printf("Invalid trial: The trial number is either not the expected value or does not match the previous guess for the current trial number\n");
        } else if (strcmp(status, "NOK") == 0) {
            printf("To make a guess you have to start a game first. Use: start PLID max_playtime\n");
        } else if (strcmp(status, "ERR") == 0) {
            printf("Invalid arguments for specified command.\n Usage: try c1 c2 c3 c4 (ci pertence {R, G, B, Y, O, P})\n");
        }
    // RQT
    } else if (sscanf(response, "RQT OK %1s %1s %1s %1s\n", c1, c2, c3, c4) == 4) {
        printf("The ongoing game has been terminated. The secret key was %s %s %s %s\n", c1, c2, c3, c4);
    } else if (sscanf(response, "RQT %s\n", status) == 1) {
        if (strcmp(status, "NOK") == 0) {
            printf("No ongoing game: The player does not have an active game to quit\n");
        } else if (strcmp(status, "ERR") == 0) {
            printf("The quit request could not be processed.\n");
        }
    // RDB
    } else if (sscanf(response, "RDB %s", status) == 1) {
        if (strcmp(status, "OK") == 0) {
            printf("New game started (max %d sec)\n", MAX_PLAYTIME);
        } else if (strcmp(status, "NOK") == 0) {
            printf("The player has an ongoing game\n");
        } else if (strcmp(status, "ERR") == 0) {
            printf("Invalid arguments for specified command.\nUsage: debug PLID (6-digit IST ID) max_playtime (up to 600 sec) c1 c2 c3 c4\n");
        }
    // RST
    } else if (sscanf(response, "RST %s %s %d", status, filename, &filesize) >= 1) {
        if (strcmp(status, "ACT") == 0 || strcmp(status, "FIN") == 0) {
            if (!save_file(response, filename, filesize)) {
                printf("Erro ao guardar o ficheiro '%s'.\n", filename);
            }
        } else if (strcmp(status, "NOK") == 0) {
            printf("Nenhum jogo ativo ou terminado encontrado para este jogador.\n");
        }
    // RSS
    } else if (sscanf(response, "RSS %s %s %d", status, filename, &filesize) >= 1) {
        if (strcmp(status, "EMPTY") == 0) {
            printf("vazio\n");
        } else if (strcmp(status, "OK") == 0) {
            if (!save_file(response, filename, filesize)) {
                printf("Erro ao guardar o ficheiro '%s'.\n", filename);
            }
        }
    // ERR
    } else {
        printf("ERR\n");
    }
}


void process_command(const char *input, char *formatted_command, char *protocol) {
    char command[BUF_SIZE];
    char c1[2], c2[2], c3[2], c4[2];

    // Análise do comando e definição do protocolo
    if (sscanf(input, "start %d %d", &PLID, &MAX_PLAYTIME) == 2) {
        snprintf(formatted_command, BUF_SIZE, "SNG %d %d\n", PLID, MAX_PLAYTIME);
        *protocol = 'U';  // UDP para o comando "start"
    } 
    // Adicionar outras condições conforme os outros comandos
    else if (sscanf(input, "try %1s %1s %1s %1s", c1, c2, c3, c4) == 4) {
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
        NUM_TRIALS = 1;
        MAX_PLAYTIME = 0;
        snprintf(formatted_command, BUF_SIZE, "QUT %d\n", PLID);
        *protocol = 'U';  // UDP para o comando "quit"
    }
    else if (strcmp(input, "exit") == 0) {
        NUM_TRIALS = 1;
        MAX_PLAYTIME = 0;
        snprintf(formatted_command, BUF_SIZE, "QUT %d\n", PLID);
        *protocol = 'U';  // UDP para o comando "exit"
        EXIT = 1;
    }
    else if (sscanf(input, "debug %d %d %1s %1s %1s %1s", &PLID, &MAX_PLAYTIME, c1, c2, c3, c4) == 6) {
        snprintf(formatted_command, BUF_SIZE, "DBG %d %d %s %s %s %s\n", PLID, MAX_PLAYTIME, c1, c2, c3, c4);
        *protocol = 'U';  // UDP para o comando "debug"
    }
    else {
        *protocol = 'X';  // X indica um comando não reconhecido
    }
}