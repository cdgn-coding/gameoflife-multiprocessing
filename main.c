#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<string.h>
#include<time.h>

const int WRITE_END = 1;
const int READ_END = 0;

enum Commands{Executing = 0};

void writeCommandToPipe(int* pipe, int command, int source, int target) {
    char buffer[100];
    sprintf(buffer, "%d,%d,%d;", command, source, target);
    printf("Writing command to pipe: %s\n", buffer);
    write(pipe[WRITE_END], buffer, strlen(buffer) + 1);
}

void closeInput(FILE* fp) {
    fclose(fp);
}

FILE* readInput(char* filename, int* nrows, int* ncolumns) {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    char buffer[100];

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    
    fscanf(fp,"%d", nrows);
    fscanf(fp,"%d", ncolumns);

    return fp;
}

void childProcess(int processNumber, int* inputPipe, int* outputPipe, int* currentRow, int* previousRow, int* nextRow, int ncols) {
    // Actualizar la generacion en currentRow
    // Mandar el resultado por el outputPipe
    printf("This is from child %d. Current = %d%d%d%d%d\n", processNumber, currentRow[0], currentRow[1], currentRow[2], currentRow[3], currentRow[4]);
}

int* lineToArray(char* line, int len) {
    int* arr = malloc(len * sizeof(int));
    char digit[] = "x";
    int j = 0;
    for (int i = 0; i < strlen(line); i++) {
        if (line[i] != ' ') {
            digit[0] = line[i];
            arr[j] = atoi(digit);
            j++;
        }
    }
    return arr;
}

void launchProcesses(int n_processes, int nrows, int ncols, FILE* file, int** inputPipes, int** outputPipes) {
    int processNumber;
    int* currentRow;
    int* previousRow;
    int* secondPreviousRow;
    char buffer[255];

    for (int i = 0; i < n_processes; ++i) {
        processNumber = i - 1;
        secondPreviousRow = previousRow;
        previousRow = currentRow;
        fgets(buffer, 255, file);
        currentRow = lineToArray(buffer, ncols);

        if (i > 0) {
            if (fork() == 0) {
                close(outputPipes[processNumber][READ_END]);
                close(inputPipes[processNumber][WRITE_END]);
                childProcess(processNumber, inputPipes[processNumber], outputPipes[processNumber], previousRow, secondPreviousRow, currentRow, ncols);
                exit(0);
            }
            else {
                close(inputPipes[processNumber][READ_END]);	
                close(outputPipes[processNumber][WRITE_END]);
            }
        }
    }

    processNumber = n_processes - 1;
    if (fork() == 0) {
        close(outputPipes[processNumber][READ_END]);
        close(inputPipes[processNumber][WRITE_END]);
        childProcess(processNumber, inputPipes[processNumber], outputPipes[processNumber], currentRow, previousRow, secondPreviousRow, ncols);
        exit(0);
    }
    else {
        close(inputPipes[processNumber][READ_END]);	
        close(outputPipes[processNumber][WRITE_END]);
    }
}

int** createPipes(int n_processes) {
    int** pipes = (int**) malloc(n_processes * sizeof(int));
    int response;
    for (int i = 0; i < n_processes; i++) {
        pipes[i] = (int*) malloc(sizeof(int) * 2);
        response = pipe(pipes[i]);
        if (response == -1) {
            perror("Error creating pipe");
            exit(1);
        }
    }
    return pipes;
}
 
int main(int argc, char *argv[])
{
    int n_processes, n_generations, n_visualizations;
    char* filename;
    int** inputPipes;
    int** outputPipes;
    int nrows, ncols;
    FILE* fp;

    n_processes = atoi(argv[1]);
    n_generations = atoi(argv[2]);
    n_visualizations = atoi(argv[3]);
    filename = argv[4];

    fp = readInput(filename, &nrows, &ncols);

    if (nrows != n_processes) {
        printf("n_processes != nrows. This is not implemented yet");
    }

    inputPipes = createPipes(n_processes);
    outputPipes = createPipes(n_processes);

    launchProcesses(n_processes, nrows, ncols, fp, inputPipes, outputPipes);
    closeInput(fp);


    // Por cada proceso i
    // Esperar por el pipe que llege el "currentRow" eso es por outputPipes[i]
    // Escribir en un archivo
    // Meter todo en un while y leer el archivo resultante en lugar del que se mando por argumento al programa

    int status;
    for (int i = 0; i < n_processes; ++i) {
        wait(&status);
    }
}