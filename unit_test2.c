#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORD_LENGTH 50 // Longitud máxima de una palabra

int main() {
    FILE *file = fopen("ca.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(file_size + 1);
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    printf("file: %p\n", (void *)file);
    printf("file_size: %d\n", file_size);
    
    // Definir el número de chunks
    int num_chunks = 4;

    // Calcular el tamaño de cada chunk
    int chunk_size = file_size / num_chunks;

    // Ajustar los tamaños de los chunks para no cortar palabras
    int start = 0;
    int end = chunk_size;
    char *buffer0 = (char *)malloc(chunk_size + 1);
    char *buffer1 = (char *)malloc(chunk_size + 1);
    char *buffer2 = (char *)malloc(chunk_size + 1);
    char *buffer3 = (char *)malloc(chunk_size + 1);
    int flag=0;

    for (int i = 0; i < num_chunks; i++) {
        // Si no es el último chunk
        if (i != num_chunks - 1) {
            // Mientras que el final del chunk actual no sea un espacio en blanco
            while (!isspace(buffer[end]) && end < file_size)
                end++;
        }
        
        // Imprimir el chunk
        printf("Chunk %d-%d: ", start, end - 1);
        if(flag==0){
            strncpy(buffer0, buffer + start, end - start);
            buffer0[end - start] = '\0';
            printf("%s\n", buffer0);
        }else if(flag==1){
            strncpy(buffer1, buffer + start, end - start);
            buffer1[end - start] = '\0';
            printf("%s\n", buffer1);
        }else if(flag==2){
            strncpy(buffer2, buffer + start, end - start);
            buffer2[end - start] = '\0';
            printf("%s\n", buffer2);
        }else if(flag==3){
            strncpy(buffer3, buffer + start, end - start);
            buffer3[end - start] = '\0';
            printf("%s\n", buffer3);
        }

        flag++;

        // Actualizar el inicio y el final del siguiente chunk
        start = end;
        end += chunk_size;
        // Ajustar el final si supera el tamaño del archivo
        if (end > file_size)
            end = file_size;
    }
    printf("Termino el programa\n");
    printf("Buffer0\n");
    printf("%s\n", buffer0);
    printf("Buffer1\n");
    printf("%s\n", buffer1);
    printf("Buffer2\n");
    printf("%s\n", buffer2);
    printf("Buffer3\n");
    printf("%s\n", buffer3);

    fclose(file);
    free(buffer);
    free(buffer0);
    free(buffer1);
    free(buffer2);
    free(buffer3);

    return 0;
}
