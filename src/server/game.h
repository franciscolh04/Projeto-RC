#ifndef GAME_H
#define GAME_H

// Definir as cores possíveis como constantes
#define TOTAL_COLORS 6
#define CODE_SIZE 4
#define MAX_PLAYS 8

// Inicializa o array de cores diretamente no cabeçalho
extern const char *colors[];

// Função para criar e armazenar a lista de cores aleatórias
void generateCode(char *selected);

// Função para verificar as cores fornecidas com a lista aleatória
void checkCode(char *secretCode, char *inputCode, int *nB, int *nW);

#endif // GAME_H
