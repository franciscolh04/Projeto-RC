#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define TOTAL_COLORS 6
#define CODE_SIZE 4

// Function to create and store the list of random colors
void generateCode(const char *colors[], const char *selected[]) {
    int i;
    // Initialize the random number generator
    srand(time(NULL));

    // Randomly select the colors
    for (i = 0; i < CODE_SIZE; i++) {
        int index = rand() % TOTAL_COLORS; // Generate a random index
        selected[i] = colors[index];
    }
}

// Function to check the provided colors against the random list
void checkColors(const char *randomColors[], const char *inputColors[], int *correctPlace, int *inList) {
    int i, j;
    *correctPlace = 0;
    *inList = 0;
    int alreadyCounted[CODE_SIZE] = {0}; // Marks the colors that have already been counted as "in the correct place"

    // Check for colors in the correct place
    for (i = 0; i < CODE_SIZE; i++) {
        if (strcmp(inputColors[i], randomColors[i]) == 0) {
            (*correctPlace)++;
            alreadyCounted[i] = 1; // Mark that this color has been counted as "in the correct place"
        }
    }

    // Check for colors that are in the list but in different positions
    for (i = 0; i < CODE_SIZE; i++) {
        if (!alreadyCounted[i]) { // Only check if it hasn't already been counted as "in the correct place"
            for (j = 0; j < TOTAL_COLORS; j++) {
                if (strcmp(inputColors[i], randomColors[j]) == 0) {
                    (*inList)++;
                    break; // No need to continue checking this color
                }
            }
        }
    }
}

int main() {
    // List of colors with initials (R, G, B, Y, O, P)
    const char *colors[TOTAL_COLORS] = {"R", "G", "B", "Y", "O", "P"}; // R = Red, G = Green, B = Blue, Y = Yellow, O = Orange, P = Purple
    
    // List to store the randomly selected colors
    const char *randomColors[CODE_SIZE];

    // Generate the random colors
    generateCode(colors, randomColors);

    // Display the selected random colors
    printf("Randomly selected colors:\n");
    for (int i = 0; i < CODE_SIZE; i++) {
        printf("%s\n", randomColors[i]);
    }

    // Example of colors provided by the user
    // (Here you can modify to receive actual user input)
    const char *inputColors[CODE_SIZE] = {"R", "B", "G", "P"}; // Example of provided colors

    int correctPlace, inList;

    // Check the provided colors
    checkColors(randomColors, inputColors, &correctPlace, &inList);

    // Display the results
    printf("\nColors in the correct place: %d\n", correctPlace);
    printf("Colors in the list but in the wrong place: %d\n", inList);

    return 0;
}
