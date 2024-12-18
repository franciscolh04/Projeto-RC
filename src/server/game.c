#include "game.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

// R = Red, G = Green, B = Blue, Y = Yellow, O = Orange, P = Purple
const char* colors[] = { "R", "G", "B", "Y", "O", "P" };

// Função para criar e armazenar a lista de cores aleatórias
void generateCode(char *selected) {
    int i;
    srand(time(NULL)); // Inicializa o gerador de números aleatórios

    for (i = 0; i < CODE_SIZE; i++) {
        int index = rand() % TOTAL_COLORS; // Gera um índice aleatório
        selected[i] = colors[index][0]; // Armazenar o primeiro caractere de cada cor
    }
    selected[CODE_SIZE] = '\0'; // Termina a string
}


// Função para verificar as cores fornecidas com a lista aleatória e retornar nB e nW
void checkCode(char *secretCode, char *inputCode, int *nB, int *nW) {
    int i, j;
    *nB = 0;  // Inicializa o número de "Black Pegs" (nB)
    *nW = 0;  // Inicializa o número de "White Pegs" (nW)

    int alreadyCounted[CODE_SIZE] = {0}; // Marca as cores que já foram verificadas como "no lugar certo"
    int usedInList[CODE_SIZE] = {0};     // Marca as cores já contadas como "na lista no lugar certo ou errado"

    // Verifica as cores no lugar certo (Black Pegs)
    for (i = 0; i < CODE_SIZE; i++) {
        if (inputCode[i] == secretCode[i]) {  // Comparação direta de caracteres
            (*nB)++;                // Incrementa o número de "Black Pegs"
            alreadyCounted[i] = 1;  // Marca como já verificada
            usedInList[i] = 1;      
        }
    }

    // Verifica as cores que estão na lista, mas em posições diferentes (White Pegs)
    for (i = 0; i < CODE_SIZE; i++) {
        if (!alreadyCounted[i]) {  // Só verifica se ainda não foi contada como "no lugar certo"
            for (j = 0; j < CODE_SIZE; j++) {
                    if (!usedInList[j] && inputCode[i] == secretCode[j]) {  // Comparação direta de caracteres
                        (*nW)++;           // Incrementa o número de "White Pegs"
                        usedInList[j] = 1; // Marca a cor como usada
                        break;             // Não precisa continuar verificando essa cor
                }
            }
        }
    }
}


