#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

// Función para leer palabras de un buffer y almacenarlas en la lista
void readBuffer(Node **head, char *buffer, int length) {
    char word[256];
    int i = 0;
    int j = 0;

    while (i < length) {
        if (isspace(buffer[i]) || ispunct(buffer[i])) {
            if (j > 0) {
                word[j] = '\0';
                addWord(head, word);
                j = 0;
            }
        } else {
            word[j++] = buffer[i];
        }
        i++;
    }

    if (j > 0) {
        word[j] = '\0';
        addWord(head, word);
    }
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

// Función para combinar listas de palabras
void mergeLists(Node **head1, Node *head2) {
    Node *current = head2;

    while (current != NULL) {
        addWord(head1, current->word);
        current = current->next;
    }
}

int main(int argc, char *argv[]) {
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

    if (rank == MANAGER) {
        FILE *file = fopen("ca.txt", "r");
        if (file == NULL) {
            perror("Error opening file");
            MPI_Finalize();
            return EXIT_FAILURE;
        }

        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        long chunk_size = file_size / 3;
        char *buffer = (char *)malloc(file_size + 1);
        fread(buffer, 1, file_size, file);
        buffer[file_size] = '\0';
        fclose(file);

        for (int i = 1; i < size; i++) {
            long start = (i - 1) * chunk_size;
            long end = (i == size - 1) ? file_size : start + chunk_size;
            long length = end - start;
            MPI_Send(&length, 1, MPI_LONG, i, 0, MPI_COMM_WORLD);
            MPI_Send(buffer + start, length, MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }

        free(buffer);

        Node *head = NULL;
        for (int i = 1; i < size; i++) {
            int count;
            MPI_Recv(&count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int j = 0; j < count; j++) {
                int word_len;
                MPI_Recv(&word_len, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                char *word = (char *)malloc(word_len + 1);
                MPI_Recv(word, word_len, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                word[word_len] = '\0';
                int word_count;
                MPI_Recv(&word_count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (int k = 0; k < word_count; k++) {
                    addWord(&head, word);
                }
                free(word);
            }
        }

        writeFile(head, "out.txt");
        freeList(head);

    } else {
        long length;
        MPI_Recv(&length, 1, MPI_LONG, MANAGER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        char *buffer = (char *)malloc(length + 1);
        MPI_Recv(buffer, length, MPI_CHAR, MANAGER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        buffer[length] = '\0';

        Node *head = NULL;
        readBuffer(&head, buffer, length);
        free(buffer);

        int count = 0;
        Node *current = head;
        while (current != NULL) {
            count++;
            current = current->next;
        }

        MPI_Send(&count, 1, MPI_INT, MANAGER, 0, MPI_COMM_WORLD);
        current = head;
        while (current != NULL) {
            int word_len = strlen(current->word);
            MPI_Send(&word_len, 1, MPI_INT, MANAGER, 0, MPI_COMM_WORLD);
            MPI_Send(current->word, word_len, MPI_CHAR, MANAGER, 0, MPI_COMM_WORLD);
            MPI_Send(&current->count, 1, MPI_INT, MANAGER, 0, MPI_COMM_WORLD);
            current = current->next;
        }

        freeList(head);
    }

    MPI_Finalize();
    return 0;
}
