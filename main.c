#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<string.h>
#include<time.h>

const int WRITE_END = 1;
const int READ_END = 0;

enum ProcessState {Created = 0, Start = 1, Starting = 2}; 
enum Commands{SETROWS = 0, SETCOLS = 1};

void writeCommandToPipe(int* pipe, int command, int source, int target) {
    char buffer[100];
    sprintf(buffer, "%d,%d,%d;", command, source, target);
    printf("Writing command to pipe: %s\n", buffer);
    write(pipe[WRITE_END], buffer, strlen(buffer) + 1);
}

void readInput(char* filename, int** pipes, int n_processes) {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    char buffer[100];
    int ncolumns, nrows;

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    
    fscanf(fp,"%d %d", &nrows, &ncolumns);

    for (int i = 0; i < n_processes; i++) {
        writeCommandToPipe(pipes[i], SETROWS, nrows, 0);
        writeCommandToPipe(pipes[i], SETCOLS, ncolumns, 0);
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        //Mandar contenido de la fila al proceso
        //printf("%s", line);
    }

    fclose(fp);
    if (line)
        free(line);
}

void childProcess(int processNumber, int** pipes) {
    enum ProcessState state = Created;
    enum Commands command;
    int source, target;
    int nrows, ncols;
    char buffer[80];
    int read_vars;

    printf("Starting process %d\n", processNumber);

    while(read(pipes[processNumber][READ_END], buffer, sizeof(buffer)) > 0) {
        printf("Process %d received: %s\n", processNumber, buffer);
        sscanf(buffer, "%d,%d,%d;\n", &command, &source, &target);
        if (command == SETROWS) {
            nrows = source;
            printf("Process %d changed nrows = %d\n", processNumber, nrows);
        }
        if (command == SETCOLS) {
            ncols = source;
            printf("Process %d changed ncols = %d\n", processNumber, ncols);
        }
    }

    printf("Closing process %d\n", processNumber);
}

void launchProcesses(int n_processes, int** pipes) {
    for (int i = 0; i < n_processes; ++i) {
        if (fork() == 0) {
		    close(pipes[i][WRITE_END]);	
            childProcess(i, pipes);
            exit(0);
        }
        else {
            close(pipes[i][READ_END]);	
        }
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
    int** pipes;

    n_processes = atoi(argv[1]);
    n_generations = atoi(argv[2]);
    n_visualizations = atoi(argv[3]);
    filename = argv[4];

    printf("n_processes = %d\n", n_processes);

    pipes = createPipes(n_processes);
    launchProcesses(n_processes, pipes);
    sleep(3);

    readInput(filename, pipes, n_processes);

    for (int i = 0; i < n_processes; i++) {
        close(pipes[i][WRITE_END]);
    }

    // wait all child processes to close
    int status;
    for (int i = 0; i < n_processes; ++i) {
        wait(&status);
    }
}