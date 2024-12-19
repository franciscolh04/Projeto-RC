#include <stdio.h>
#include "verifications.h"

// Função que verifica se as cores são válidas
int valid_colors( char *color_code) {
    const char colors[] = { 'R', 'G', 'B', 'Y', 'O', 'P' };
    for (int i = 0; i < 4; i++) {
        int valid = 0;
        for (int j = 0; j < 6; j++) {
            if (color_code[i] == colors[j]) {
                valid = 1;
                break;
            }
        }
        if (!valid) {
            return 0; // Retorna 0 se qualquer letra for inválida
        }
    }
    return 1; // Retorna 1 se todas forem válidas
}

int count_digits(int num) {
    if (num == 0) { 
        return 1;
    }
    int count = 0;
    while (num > 0) {
        num /= 10; // Remove o último dígito
        count++;
    }
    return count;
}