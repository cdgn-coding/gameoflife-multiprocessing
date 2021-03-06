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
int LOG_LEVEL = LOG;

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
    char* str = (char*) malloc(MAX_BUFFER_SIZE * sizeof(char));
    sprintf(str, "/tmp/output_to_main_%d", processNumber);
    return str;
}

/**
 * Outputs the name of the fifo that sends messages from source to target
 */
char* interChildProcessPipeName(int sourceProcess, int targetProcess) {
    char* str = (char*) malloc(MAX_BUFFER_SIZE * sizeof(char));
    sprintf(str, "/tmp/messages_from_%d_to_%d", sourceProcess, targetProcess);
    return str;
}

/**
 * Create n_processes unnamed pipes
 */ 
void createPipes(int n_processes, char* logDetail) {
    int response;
    char errorMsg[MAX_BUFFER_SIZE];
    char* filepath;

    for (int i = 0; i < n_processes; i++) {
        // Create FIFO to send data from main to the process
        filepath = inputFromMainPipeName(i);
        response = mkfifo(filepath, 0777);
        if (response == -1) {
            printf("Error opening pipe %s. errno=%d\n", filepath, errno);
        }
        free(filepath);

        // Create FIFO to send data from process to main
        filepath = outputToMainPipeName(i);
        response = mkfifo(filepath, 0777);
        if (response == -1) {
            printf("Error opening pipe %s.\n", filepath);
        }
        free(filepath);

        // Create FIFO to send data from this process to the next one
        if (i < n_processes - 1) {
            filepath = interChildProcessPipeName(i, i + 1);
            if (LOG_LEVEL >= LOG) {
                printf("Opening FIFO %s\n", filepath);
            }
            if (response == -1) {
                printf("Error opening pipe %s.\n", filepath);
            }
            free(filepath);
        }

        // Create FIFO to send data from this process to the previous one
        if (i > 0) {
            filepath = interChildProcessPipeName(i, i - 1);
            if (LOG_LEVEL >= LOG) {
                printf("Opening FIFO %s\n", filepath);
            }
            if (response == -1) {
                printf("Error opening pipe %s.\n", filepath);
            }
            free(filepath);
        }
    }
}

/**
 * Open FIFOs from subprocesses, messages sent from main
 */
int* getSubprocessInputWriterFromMain(int n_processes) {
    int* fifos = (int*) malloc(n_processes * sizeof(int));
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
    if (LOG_LEVEL >= DEBUG) {
        printf("Process %d opening FIFO %s\n", processNumber, fifoName);
    }
    response = open(fifoName, O_RDONLY);
    if ((response == -1) && (LOG_LEVEL >= ERROR)) {
        printf("Error opening FIFO named %s. errno=%d\n", fifoName, errno);
    }
    free(fifoName);
    return response;
}

int getInterprocessFIFOs(int sourceProcess, int targetProcess) {
    int response;
    char* fifoName = interChildProcessPipeName(sourceProcess, targetProcess);
    if (LOG_LEVEL >= DEBUG) {
        printf("Process %d opening FIFO %s\n", sourceProcess, fifoName);
    }
    response = open(fifoName, O_RDONLY);
    if ((response == -1) && (LOG_LEVEL >= ERROR)) {
        printf("Error opening FIFO named %s. errno=%d\n", fifoName, errno);
    }
    free(fifoName);
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

void matrixToString(int** arr, int rows, int cols, char* dest) {
    char aux[MAX_BUFFER_SIZE] = "";
    sprintf(dest, "");
    for (int k = 0; k < rows; k++) {
        for (int j = 0; j < cols; j++) {
            sprintf(aux, "%s%d", dest, arr[k][j]);
            strcpy(dest, aux);
        }
        if (k < rows - 1) {
            sprintf(aux, "%s; ", dest);
            strcpy(dest, aux);
        }
    }
}

/**
 * Write array to pipe
 */
void writeArray(int pipeWriteEnd, int* arr, int len) {
    char* package = calloc(len + 1, sizeof(char));

    for (int k = 0; k < len; k++) {
        package[k] = arr[k] + '0';
    }
    package[len] = '\0';

    if (LOG_LEVEL >= DEBUG) {
        printf("Content to be sent to pipe %d: \"%s\"\n", pipeWriteEnd, package);
    }

    write(pipeWriteEnd, package, len);
    free(package);
}

/**
 * Read array from pipe
 */
void readArray(int pipeReadEnd, int len, int* dest, char* logDetail) {
    int response;
    char* package;
    int package_size = len + 1;

    package = (char*) malloc(package_size * sizeof(char));
    package[len] = '\0';
    response = read(pipeReadEnd, package, len);

    if (LOG_LEVEL >= DEBUG) {
        printf("%s Content read from pipe %d: \"%s\", got response=%d (expected %d), errno=%d\n", logDetail, pipeReadEnd, package, response, len, errno);
    }

    if ((response == -1) && (LOG_LEVEL >= ERROR)) {
        printf("%s When reading pipe, got response=%d. errno=%d\n", logDetail, response, errno);
    }

    for (int k = 0; k < len; k++) {
        dest[k] = package[k] - '0';
    }
    free(package);
}

/**
 * Process function, solves game of life for a matrix chunk
 */
void gameOfLifeSubprocess(
    int n_generations,
    int n_visualizations,
    int n_processes,
    int processNumber,
    int rowsOfProcess,
    int ncols,
    int inputPipeFromMain,
    int inputPipeFromNext,
    int inputPipeFromPrev,
    int outputPipeToNext,
    int outputPipeToPrev
) {
    char rowContent[MAX_BUFFER_SIZE];
    char logDetail[MAX_BUFFER_SIZE];
    sprintf(logDetail, "Process %d.", processNumber);
    int** submatrix = (int**) malloc(rowsOfProcess * sizeof(int*));

    for (int i = 0; i < rowsOfProcess; i++) {
        if (LOG_LEVEL >= DEBUG) {
            printf("%s Reading row %d of matrix chunk\n", logDetail, i);
        }
        submatrix[i] = (int*) malloc(ncols * sizeof(int));
        readArray(inputPipeFromMain, ncols, submatrix[i], logDetail);
    }

    // Be careful of long matrices
    if (LOG_LEVEL >= DEBUG) {
        matrixToString(submatrix, rowsOfProcess, ncols, rowContent);
        printf("%s... Received \"%s\"\n", logDetail, rowContent);
    }

    // Run the game of life in the chunk matrix
    int* currentRow;
    int* nextRowToSubmatrix = malloc(ncols * sizeof(int));
    int* previousRowToSubmatrix = malloc(ncols * sizeof(int));
    int* previousRow;
    int* nextRow;
    int hasPrevRow;
    int hasNextRow;

    for (int currentGeneration = 0; currentGeneration < n_generations; currentGeneration++) {
        if (processNumber > 0) {
            writeArray(outputPipeToPrev, submatrix[0], ncols);
            readArray(inputPipeFromPrev, ncols, previousRowToSubmatrix, logDetail);
        }

        if (processNumber < n_processes - 1) {
            writeArray(outputPipeToNext, submatrix[rowsOfProcess - 1], ncols);
            readArray(inputPipeFromNext, ncols, nextRowToSubmatrix, logDetail);
        }

        for (int row = 0; row < rowsOfProcess; row++) {
            if ((processNumber == 0) && (row == 0)) {
                hasPrevRow = FALSE;
            } else if (row > 0) {
                previousRow = submatrix[row - 1];
            } else {
                previousRow = previousRowToSubmatrix;
            }

            if ((processNumber == n_processes - 1) && (row == rowsOfProcess - 1)) {
                hasNextRow = FALSE;
            } else if (row < rowsOfProcess - 1) {
                nextRow = submatrix[row + 1];
            } else {
                previousRow = previousRowToSubmatrix;
            }

            for (int col = 0; col < ncols; col++) {

            }
        }
    }

    free(nextRowToSubmatrix);
    free(previousRowToSubmatrix);
    // Freeing mem and closing files
    for (int i = 0; i < rowsOfProcess; i++) {
        free(submatrix[i]);
    }
    free(submatrix);
    if (LOG_LEVEL >= INFO) {
        printf("%s Exiting correctly.\n", logDetail);
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
        int inputPipeFromNext;
        int inputPipeFromPrev;
        int outputPipeToPrev;
        int outputPipeToNext;

        if ((pid = fork()) == 0) {
            int inputFromMain = getSubprocessInputReaderFromMain(i);
            if (LOG_LEVEL >= DEBUG) {
                printf("Process %d inputFromMain=%d\n", i, inputFromMain);
            }

            inputPipeFromPrev = i > 0 ? getInterprocessFIFOs(i - 1, i) : -1;
            inputPipeFromNext = i < n_processes - 1 ? getInterprocessFIFOs(i + 1, i) : -1;
            outputPipeToPrev = i > 0 ? getInterprocessFIFOs(i, i - 1) : -1;
            outputPipeToNext = i < n_processes - 1 ? getInterprocessFIFOs(i, i + 1) : -1;

            gameOfLifeSubprocess(
                n_generations,
                n_visualizations,
                n_processes,
                i,
                rowsOfProcess,
                ncols,
                inputFromMain,
                inputPipeFromNext,
                inputPipeFromPrev,
                outputPipeToNext,
                outputPipeToPrev
            );
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
            free(matrixRow);

            if (LOG_LEVEL >= DEBUG) {
                printf("Reading line %d\n", lineCounter);
                lineCounter++;
            }

            if (LOG_LEVEL >= DEBUG) {
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

    if (LOG_LEVEL >= LOG) {
        printf("Computing parameters...\n");
    }

    int rowsPerProcess = nrows / n_processes;
    int firstProcessRows = rowsPerProcess + (nrows % n_processes);

    if (LOG_LEVEL >= LOG) {
        printf("Parameters: nrows=%d, ncols=%d, n_processes=%d, rowsPerProcess=%d, firstProcessRows=%d\n", nrows, ncols, n_processes, rowsPerProcess, firstProcessRows);
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

    if (LOG_LEVEL >= LOG) {
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

    if (LOG_LEVEL >= LOG) {
        printf("Game of Life initiating...\n");
    }

    if (LOG_LEVEL >= LOG) {
        printf("Terminated Game of Life, exiting...\n");
    }
    free(inputsFromMain);
    fclose(inputFile);
    for (int k = 0; k < n_processes; k++) {
        close(inputsFromMain[k]);
    }
}

