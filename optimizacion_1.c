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

/* void readBuffer(Node **head, char *buffer, int length) {
    char *word = (char *)malloc((length + 1) * sizeof(char)); // Buffer dinámico
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

    free(word); // Liberar la memoria del buffer dinámico
} */


void readBuffer(Node **head, const char *buffer) {
    const char *delimiter = " \t\n"; // Delimitadores para palabras
    char *word = strtok(strdup(buffer), delimiter); // Duplicar buffer para evitar modificarlo
    while (word != NULL) {
        addWord(head, word);
        word = strtok(NULL, delimiter);
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
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';
    fclose(file);

    // Ajustar los tamaños de los chunks para no cortar palabras
    int lengths[size];
    int displs[size];
    displs[0] = 0;
    for (int i = 0; i < size; i++) {
        int end = (i == size - 1) ? file_size : displs[i] + chunk_size;
        while (end < file_size && !isspace(buffer[end])) {
            end++;
        }
        lengths[i] = end - displs[i];
        if (i < size - 1) {
            displs[i + 1] = end;
        }
    }

    int recv_length;
    char *local_chunk = (char *)malloc(lengths[rank] + 1);
    MPI_Scatter(lengths, 1, MPI_INT, &recv_length, 1, MPI_INT, MANAGER, MPI_COMM_WORLD);
    MPI_Scatterv(buffer, lengths, displs, MPI_CHAR, local_chunk, recv_length, MPI_CHAR, MANAGER, MPI_COMM_WORLD);
    local_chunk[recv_length] = '\0';


    if(rank==0){
        printf("Hola soy master trabajando\n");
        Node *head0 = NULL;
        
        readBuffer(&head0,local_chunk);
        writeFile(head0, "master.txt");

        //printf("Chunk del proceso %d:\n%s\n", rank, local_chunk);
        freeList(head0);

    }else if(rank==1){
        Node *head1 = NULL;

        printf("Hola soy esclavo 1\n");

        readBuffer(&head1,local_chunk);

        writeFile(head1, "slave_1.txt");

       // printf("Chunk del proceso %d:\n%s\n", rank, local_chunk);
        freeList(head1);

    }else if(rank==2){
        Node *head2 = NULL;

        printf("Hola soy esclavo 2\n");
        readBuffer(&head2,local_chunk);
        //printf("Chunk del proceso %d:\n%s\n", rank, local_chunk);
        writeFile(head2, "slave_2.txt");

        freeList(head2);

        
    }else if(rank==3){
        Node *head3 = NULL;

        readBuffer(&head3,local_chunk);
        printf("Hola soy esclavo 3\n");
        writeFile(head3, "slave_3.txt");


        printf("Chunk del proceso %d:\n%s\n", rank, local_chunk);

        freeList(head3);
        
    }

    free(buffer);
    free(local_chunk);
    MPI_Finalize();
    return 0;
}
