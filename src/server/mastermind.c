#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define TOTAL_CORES 6
#define TOTAL_SELECIONADAS 4

// Função para criar e armazenar a lista de cores aleatórias
void criarCoresAleatorias(const char *cores[], const char *selecionadas[]) {
    int i;
    // Inicializa o gerador de números aleatórios
    srand(time(NULL));

    // Escolhe aleatoriamente as cores
    for (i = 0; i < TOTAL_SELECIONADAS; i++) {
        int indice = rand() % TOTAL_CORES; // Gera um índice aleatório
        selecionadas[i] = cores[indice];
    }
}

// Função para verificar as cores fornecidas com a lista aleatória
void verificarCores(const char *coresAleatorias[], const char *coresInput[], int *noLugarCerto, int *naLista) {
    int i, j;
    *noLugarCerto = 0;
    *naLista = 0;
    int jaContada[TOTAL_SELECIONADAS] = {0}; // Marca as cores que já foram verificadas como "no lugar certo"

    // Verifica as cores no lugar certo
    for (i = 0; i < TOTAL_SELECIONADAS; i++) {
        if (strcmp(coresInput[i], coresAleatorias[i]) == 0) {
            (*noLugarCerto)++;
            jaContada[i] = 1; // Marca que essa cor já foi contada como "no lugar certo"
        }
    }

    // Verifica as cores que estão na lista, mas em posições diferentes
    for (i = 0; i < TOTAL_SELECIONADAS; i++) {
        if (!jaContada[i]) { // Só verifica se ainda não foi contada como "no lugar certo"
            for (j = 0; j < TOTAL_CORES; j++) {
                if (strcmp(coresInput[i], coresAleatorias[j]) == 0) {
                    (*naLista)++;
                    break; // Não precisa continuar verificando essa cor
                }
            }
        }
    }
}

int main() {
    // Lista de cores com iniciais (R, G, B, Y, O, P)
    const char *cores[TOTAL_CORES] = {"R", "G", "B", "Y", "O", "P"}; // R = Red, G = Green, B = Blue, Y = Yellow, O = Orange, P = Purple
    
    // Lista para armazenar as cores aleatórias escolhidas
    const char *coresAleatorias[TOTAL_SELECIONADAS];

    // Gera as cores aleatórias
    criarCoresAleatorias(cores, coresAleatorias);

    // Exibe as cores aleatórias geradas
    printf("Cores aleatórias selecionadas:\n");
    for (int i = 0; i < TOTAL_SELECIONADAS; i++) {
        printf("%s\n", coresAleatorias[i]);
    }

    // Exemplo de cores fornecidas pelo usuário
    // (Aqui você pode modificar para receber a entrada real do usuário)
    const char *coresInput[TOTAL_SELECIONADAS] = {"R", "B", "G", "P"}; // Exemplo de cores fornecidas

    int noLugarCerto, naLista;

    // Verifica as cores fornecidas
    verificarCores(coresAleatorias, coresInput, &noLugarCerto, &naLista);

    // Exibe os resultados
    printf("\nCores no lugar certo: %d\n", noLugarCerto);
    printf("Cores na lista, mas em lugar errado: %d\n", naLista);

    return 0;
}