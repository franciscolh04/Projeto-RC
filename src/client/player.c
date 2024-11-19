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

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT + GN;
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];
    char *server_ip = "127.0.0.1";  // IP do servidor (localhost por padrão)
    int opt;
    socklen_t server_len = sizeof(server_addr);
    
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

    // Criar socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configurar endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);  // Porta do servidor
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Erro ao configurar o endereço IP do servidor");
        exit(EXIT_FAILURE);
    }

    printf("Conectado ao servidor %s na porta %d...\n", server_ip, port);

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
    
        
        if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Erro ao enviar mensagem para o servidor");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        printf("Mensagem enviada para o servidor: %s\n", buffer);
        

        // Comando inserido pelo utilizador
        // Switch para ver que comando é e preparar mensagem a enviar
        // Enviar comando no formato correto para o servidor


        // Esperar pela resposta do servidor
        int n = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&server_addr, &server_len);
        if (n < 0) {
            perror("Erro ao receber resposta do servidor");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        buffer[n] = '\0';  // Garantir que a resposta seja uma string válida
        printf("Resposta recebida do servidor: %s\n", buffer);
    }

    // Fechar o socket após sair do loop
    close(sockfd);

    return 0;
}
