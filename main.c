#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include<errno.h>
#include<time.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_BUFFER_SIZE 255

const int WRITE_END = 1;
const int READ_END = 0;

const int FALSE = 0;
const int TRUE = 1;

const int DEBUG = 3;
const int INFO = 2;
const int LOG = 1;
const int ERROR = 0;
const int NO_LOGS = -1;
const int LOG_LEVEL = INFO;

/**
 * Outputs the name of the fifo that inputs from processes to the main process
 */
char* inputFromMainPipeName(int processNumber) {
    char* str = calloc(MAX_BUFFER_SIZE, sizeof(char));
    sprintf(str, "/tmp/input_from_main_%d", processNumber);
    return str;
}

/**
 * Outputs the name of the fifo that outputs from processes to the main process
 */
char* outputToMainPipeName(int processNumber) {
    char* str = malloc(MAX_BUFFER_SIZE * sizeof(char));
    sprintf(str, "/tmp/output_to_main_%d", processNumber);
    return str;
}

/**
 * Create n_processes unnamed pipes
 */ 
void createPipes(int n_processes, char* logDetail) {
    int response;
    char errorMsg[MAX_BUFFER_SIZE];

    for (int i = 0; i < n_processes; i++) {
        mkfifo(inputFromMainPipeName(i), 0777);
        mkfifo(outputToMainPipeName(i), 0777);
    }
}

/**
 * Open FIFOs from subprocesses, messages sent from main
 */
int* getSubprocessInputWriterFromMain(int n_processes) {
    int* fifos = malloc(n_processes * sizeof(int));
    char* fifoName;

    for (int i = 0; i < n_processes; i++) {
        fifoName = inputFromMainPipeName(i);
        if (LOG_LEVEL >= DEBUG) {
            printf("Opening FIFO %s\n", fifoName);
        }
        fifos[i] = open(fifoName, O_WRONLY);
    }

    return fifos;
}

int getSubprocessInputReaderFromMain(int processNumber) {
    int response;
    char* fifoName = inputFromMainPipeName(processNumber);
    response = open(fifoName, O_RDONLY);
    if ((response == -1) && (LOG_LEVEL >= ERROR)) {
        printf("Error opening FIFO named %s. errno=%d\n", fifoName, errno);
    }
    return response;
}

void arrayToString(int* arr, int len, char* dest) {
    char aux[MAX_BUFFER_SIZE] = "";
    sprintf(dest, "");
    for (int k = 0; k < len; k++) {
        sprintf(aux, "%s%d", dest, arr[k]);
        strcpy(dest, aux);
    }
}

/**
 * Write array to pipe
 */
void writeArray(int pipeWriteEnd, int* arr, int len) {
    int package_size = len + 1;
    char* package = calloc(len + 1, sizeof(char));

    for (int k = 0; k < len; k++) package[k] = arr[k] + '0';
    package[len] = '\0';

    if (LOG_LEVEL >= INFO) {
        printf("Content to be sent to pipe: \"%s\"\n", package);
    }

    write(pipeWriteEnd, package, package_size);
}

/**
 * Read array from pipe
 */
int* readArray(int pipeReadEnd, int len, char* logDetail) {
    int* arr = calloc(len, sizeof(int));
    char* package = calloc(len, sizeof(char));
    int response;

    response = read(pipeReadEnd, package, len);
    if (LOG_LEVEL >= INFO) {
        printf("%s Content read from pipe: \"%s\", got response=%d (expected %d), errno=%d\n", logDetail, package, response, len, errno);
    }

    if ((response == -1) && (LOG_LEVEL >= ERROR)) {
        printf("%s When reading pipe, got response=%d. errno=%d\n", logDetail, response, errno);
    }

    for (int k = 0; k < len; k++) arr[k] = package[k] - '0';
    return arr;
}

/**
 * Process function, solves game of life for a matrix chunk
 */
void gameOfLifeSubprocess(
    int processNumber,
    int rowsOfProcess,
    int ncols,
    int inputPipeFromMain
) {
    int** submatrix = malloc(rowsOfProcess * sizeof(int));
    char rowContent[MAX_BUFFER_SIZE];
    char logDetail[MAX_BUFFER_SIZE];
    sprintf(logDetail, "Process %d.", processNumber);

    for (int i = 0; i < rowsOfProcess; i++) {
        submatrix[i] = readArray(inputPipeFromMain, ncols, logDetail);
    }

    if (LOG_LEVEL >= LOG) {
        for (int i = 0; i < rowsOfProcess; i++) {
            arrayToString(submatrix[0], ncols, rowContent);
            printf("Row %d. %s is \"%s\"\n", i, logDetail, rowContent);
        }
    }
}

/**
 * Launches processes for every block
 */ 
void launchProcesses(
    FILE* inputFile,
    int n_processes,
    int rowsPerProcess,
    int firstProcessRows,
    int ncols,
    int n_generations,
    int n_visualizations
) {
    int pid;

    for (int i = 0; i < n_processes; i++) {
        int rowsOfProcess = i == 0 ? firstProcessRows : rowsPerProcess;

        if ((pid = fork()) == 0) {
            int inputFromMain = getSubprocessInputReaderFromMain(i);
            gameOfLifeSubprocess(i, rowsOfProcess, ncols, inputFromMain);
            exit(0);
        }
        else if (pid < 0) {
            printf("Error launching process %d\n", i);
        }
    }
}

/**
 * Read nrows and ncols from the file
 * Return the file poiting to the matrix
 */ 
FILE* startReadingInput(char* filename, int* nrows, int* ncolumns) {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    fscanf(fp,"%d\n", nrows);
    fscanf(fp,"%d\n", ncolumns);

    return fp;
}

/**
 * Converts a string of the file to an array of numbers
 */
int* lineToArray(char* line, int len) {
    int* arr = calloc(len, sizeof(int));
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

/**
 * Continue reading values from the file as a matrix
 * Pass to processes, value by value
 */
void continueReadingInput(
    FILE* inputFile,
    int* inputPipesFromMain,
    int n_processes,
    int nrows,
    int ncols,
    int rowsPerProcess,
    int firstProcessRows
) {
    int numberToRead;
    int* matrixRow;
    char buffer[MAX_BUFFER_SIZE];
    int lineCounter = 0;

    for (int i = 0; i < n_processes; i++) {
        int rowsToRead = i == 0 ? firstProcessRows : rowsPerProcess;
        for (int j = 0; j < rowsToRead; j++) {
            fgets(buffer, MAX_BUFFER_SIZE, inputFile);
            matrixRow = lineToArray(buffer, ncols);
            arrayToString(matrixRow, ncols, buffer);
            writeArray(inputPipesFromMain[i], matrixRow, ncols);

            if (LOG_LEVEL >= DEBUG) {
                printf("Reading line %d\n", lineCounter);
                lineCounter++;
            }

            if (LOG_LEVEL >= INFO) {
                printf("Content sent to process %d is \"%s\"\n", i, buffer);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int n_processes, n_generations, n_visualizations;
    char* filename;
    FILE* inputFile;
    char logDetail[MAX_BUFFER_SIZE];
    int* inputsFromMain;

    // Read arguments
    n_processes = atoi(argv[1]);
    n_generations = atoi(argv[2]);
    n_visualizations = atoi(argv[3]);
    filename = argv[4];

    int nrows, ncols;

    if (LOG_LEVEL >= INFO) {
        printf("Starting to read input...\n");
    }
    inputFile = startReadingInput(filename, &nrows, &ncols);

    if (LOG_LEVEL >= INFO) {
        printf("Procceeding to create FIFOs...\n");
    }
    createPipes(n_processes, logDetail);

    int rowsPerProcess = nrows / n_processes;
    int firstProcessRows = rowsPerProcess + (n_processes % nrows);

    if (LOG_LEVEL >= LOG) {
        printf("nrows=%d, ncols=%d, n_processes=%d, rowsPerProcess=%d, firstProcessRows=%d\n", nrows, ncols, n_processes, rowsPerProcess, firstProcessRows);
    }

    if (LOG_LEVEL >= INFO) {
        printf("Procceeding to launch processes\n");
    }

    launchProcesses(
        inputFile,
        n_processes,
        rowsPerProcess,
        firstProcessRows,
        ncols,
        n_generations,
        n_visualizations
    );

    if (LOG_LEVEL >= DEBUG) {
        printf("Procceeding to open FIFOs to pass data to the subprocesses...\n");
    }
    // Open FIFOS to pass data to the subprocesses
    inputsFromMain = getSubprocessInputWriterFromMain(n_processes);

    if (LOG_LEVEL >= DEBUG) {
        printf("Procceeding to read the rest of the input...\n");
    }

    continueReadingInput(
        inputFile,
        inputsFromMain,
        n_processes,
        nrows,
        ncols,
        rowsPerProcess,
        firstProcessRows
    );
}

