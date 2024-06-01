#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main() {
    FILE *file = fopen("ca.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    int chunk_size = file_size / 4;

    char *buffer = (char *)malloc(file_size + 1);
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    printf("file: %p\n", (void *)file);
    printf("file_size: %d\n", file_size);
    printf("chunk_size: %d\n", chunk_size);
    printf("buffer: %s\n", buffer);

    fclose(file);
    free(buffer);

    return 0;
}
