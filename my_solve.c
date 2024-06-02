#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mpi.h>

#define MANAGER 0

// Estructura del nodo para almacenar una palabra y su concurrencia
typedef struct Node {
    char *word;
    int count;
    struct Node *next;
} Node;

// Función para crear un nuevo nodo
Node* createNode(char *word) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->word = strdup(word);
    newNode->count = 1;
    newNode->next = NULL;
    return newNode;
}

// Función para agregar una palabra a la lista
void addWord(Node **head, char *word) {
    Node *current = *head;
    Node *previous = NULL;

    // Verificar duplicados por mayúsculas
    for (int i = 0; word[i]; i++) {
        word[i] = tolower(word[i]);
    }

    // Buscar si la palabra ya está en la lista
    while (current != NULL && strcmp(current->word, word) != 0) {
        previous = current;
        current = current->next;
    }

    if (current == NULL) {
        // Si la palabra no está en la lista, agregar un nuevo nodo
        Node *newNode = createNode(word);
        if (previous == NULL) {
            *head = newNode;
        } else {
            previous->next = newNode;
        }
    } else {
        // Si la palabra ya está en la lista, incrementar el conteo
        current->count++;
    }
}

// Función para leer palabras de un archivo y almacenarlas en la lista
void readFile(Node **head, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char word[256];
    while (fscanf(file, "%255s", word) != EOF) {
        addWord(head, word);
    }

    fclose(file);
}

// Función para escribir la lista de palabras y sus conteos en un archivo
void writeFile(Node *head, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    Node *current = head;
    while (current != NULL) {
        fprintf(file, "%s: %d\n", current->word, current->count);
        current = current->next;
    }

    fclose(file);
}

// Función para liberar la memoria de la lista
void freeList(Node *head) {
    Node *current = head;
    Node *nextNode;

    while (current != NULL) {
        nextNode = current->next;
        free(current->word);
        free(current);
        current = nextNode;
    }
}

int main(int argc, char *argv[]) {
    Node *head = NULL;

    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 4) {
        if (rank == MANAGER) {
            fprintf(stderr, "Este programa necesita 4 procesos.\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int file_size, chunk_size;
    char *buffer = NULL;
    char *buffer0 = NULL;
    char *buffer1 = NULL;
    char *buffer2 = NULL;
    char *buffer3 = NULL;

    if (rank == MANAGER) {

        FILE *file = fopen("ca.txt", "r");
        if (file == NULL) {
            perror("Error opening file");
            MPI_Finalize();
            return EXIT_FAILURE;
        }

        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        chunk_size = file_size / size;

        
        buffer = (char *)malloc(file_size + 1);

        char *buffer0 = (char *)malloc(chunk_size + 1);
        buffer1 = (char *)malloc(chunk_size + 1);
        buffer2 = (char *)malloc(chunk_size + 1);
        buffer3 = (char *)malloc(chunk_size + 1);

        fread(buffer, 1, file_size, file);
        buffer[file_size] = '\0';
        fclose(file);

        int num_chunks=4;    

        // Ajustar los tamaños de los chunks para no cortar palabras
        int start = 0;
        int end = chunk_size;
        int flag=0;

        for (int i = 0; i < num_chunks; i++) {
            // Si no es el último chunk
            if (i != num_chunks - 1) {
                // Mientras que el final del chunk actual no sea un espacio en blanco
                while (!isspace(buffer[end]) && end < file_size)
                    end++;
            }
            
            if(flag==0){
                strncpy(buffer0, buffer + start, end - start);
                buffer0[end - start] = '\0';
            }else if(flag==1){
                strncpy(buffer1, buffer + start, end - start);
                buffer1[end - start] = '\0';
            }else if(flag==2){
                strncpy(buffer2, buffer + start, end - start);
                buffer2[end - start] = '\0';
            }else if(flag==3){
                strncpy(buffer3, buffer + start, end - start);
                buffer3[end - start] = '\0';
            }
            flag++;
            // Actualizar el inicio y el final del siguiente chunk
            start = end;
            end += chunk_size;
            // Ajustar el final si supera el tamaño del archivo
            if (end > file_size)
                end = file_size;
        }

        int lengths[4] = {strlen(buffer0), strlen(buffer1), strlen(buffer2), strlen(buffer3)}; 


        
        MPI_Send(&lengths[0], 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Send(buffer0, lengths[0], MPI_CHAR, 1, 0, MPI_COMM_WORLD);

        MPI_Send(&lengths[1], 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
        MPI_Send(buffer1, lengths[1], MPI_CHAR, 2, 0, MPI_COMM_WORLD);

        MPI_Send(&lengths[2], 1, MPI_INT, 3, 0, MPI_COMM_WORLD);

        MPI_Send(buffer2, lengths[2], MPI_CHAR, 3, 0, MPI_COMM_WORLD);

       printf("Chunk del proceso %d:\n%s\n", MANAGER, buffer3);



    }else {
        int recv_length;
        MPI_Recv(&recv_length, 1, MPI_INT, MANAGER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        char *local_chunk = (char *)malloc(recv_length + 1);
        local_chunk[recv_length] = '\0';
        MPI_Recv(local_chunk, recv_length, MPI_CHAR, MANAGER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Chunk del proceso %d:\n%s\n", rank, local_chunk);
        free(local_chunk);
    }

     if (rank == 0) {
        free(buffer);
        free(buffer0);
        free(buffer1);
        free(buffer2);
        free(buffer3);
    }


    MPI_Finalize();
    return 0;
}