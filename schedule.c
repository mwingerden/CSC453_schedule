#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>
//TODO: Memory check. Ex: check result in getMethods is null
//TODO: Test program where the user didn't input any :
//TODO: Test program with 100 methods

#define MAX_PROCESSES 150
#define MAX_ARGUMENTS 10

struct MethodCall {
    char methodName[50];
    char *arguments[MAX_ARGUMENTS + 1];
};

int numProcesses = 0;
int quantum = 0;
int currentProcess = 0;
pid_t childPIDs[MAX_PROCESSES];

void numberOfChildren(int argc, char *argv[]) {
    int semicolonCount = 1;
    int i = 0;

    for (; i < argc; i++) {
        if (!strcmp(argv[i], ":")) {
            semicolonCount++;
        }
    }

    numProcesses = semicolonCount;
}

void getQuantum(char *argv[]) {
    quantum = (int) strtol(argv[1], NULL, 10);
}

struct MethodCall *createMethods(int numMethods, int argc, char *argv[]) {
    size_t totalSize = sizeof(struct MethodCall) * numMethods;

    struct MethodCall *methods = (struct MethodCall *) mmap(
            NULL,
            totalSize,
            PROT_WRITE, // Set the appropriate permissions
            MAP_SHARED | MAP_ANONYMOUS,
            -1,
            0
    );

    if (methods == MAP_FAILED) {
        perror("Failed mmap in getMethods\n");
        exit(1);
    }

    int i, j;
    for (i = 0; i < numMethods; i++) {
        for (j = 0; j < MAX_ARGUMENTS + 1; j++) {
            methods[i].arguments[j] = NULL;
        }
    }

    char methodName[50] = "";
    int structMethodCount = 0;
    int structArgumentCount = 1;
    int methodCount = 2;
    int argumentCount = 3;
    for (; structMethodCount < numMethods; structMethodCount++) {
        if(structMethodCount >= MAX_PROCESSES) {
            printf("To many methods. Will not add: %s\n", argv[methodCount]);
        }
        else {
            for (; methodCount < argc;) {
                strcpy(methods[structMethodCount].methodName, argv[methodCount]);
                methods[structMethodCount].arguments[0] = argv[methodCount];
                for (; argumentCount < argc; argumentCount++) {
                    if (strcmp(argv[argumentCount], ":") != 0) {
                        if(structArgumentCount > MAX_ARGUMENTS) {
                            printf("To many arguments. Will not add: %s\n", argv[argumentCount]);
                            structArgumentCount++;
                        }
                        else {
                            methods[structMethodCount].arguments[structArgumentCount++] = argv[argumentCount];
                        }
                    } else {
                        methods[structMethodCount].arguments[structArgumentCount] = NULL;
                        methodCount += 1 + structArgumentCount;
                        argumentCount += 2;
                        structArgumentCount = 1;
                        strcpy(methodName, "");
                        break;
                    }
                }
                break;
            }
        }

    }

    return methods;
}

void signalHandler() {
}

void sigalrmHandler(int signum) {
    kill(childPIDs[currentProcess], SIGSTOP);
}

void roundRobinSchedule(struct MethodCall *methods) {
    int i;

    for (i = 0; i < numProcesses; i++) {
        childPIDs[i] = fork();

        if (childPIDs[i] == -1) {
            perror("Fork failed");
            exit(1);
        }

        if (childPIDs[i] == 0) {
            signal(SIGCONT, signalHandler);

            pause();

            if (execvp(methods[i].methodName, methods[i].arguments) == -1) {
                perror("execvp");
                fprintf(stderr, "Failed to execute: %s\n", methods[i].methodName);
                exit(EXIT_FAILURE);
            }

            exit(0);
        }
    }

    sleep(1);

    signal(SIGALRM, sigalrmHandler);

    currentProcess = 0;
    int processFinished = 0;
    int status;
    int processCheck[numProcesses];
    for (i = 0; i < numProcesses; i++) {
        processCheck[i] = 0;
    }
    alarm(quantum / 1000);
    kill(childPIDs[currentProcess], SIGCONT);
    while(processFinished < numProcesses) {
        printf("Waiting for Process: %d\n", currentProcess + 1);
        fflush(stdout);
        pid_t temp = waitpid(childPIDs[currentProcess], &status, WUNTRACED);
        printf("Child Response: %d\n", temp);
        fflush(stdout);

        if (WIFSTOPPED(status)) {
            currentProcess = (currentProcess + 1) % numProcesses;
            printf("Switched Processes to: %d\n", currentProcess + 1);
            fflush(stdout);
            alarm(quantum / 1000);
            kill(childPIDs[currentProcess], SIGCONT);
            printf("Started current Process: %d\n", currentProcess + 1);
            fflush(stdout);
        } else if (WIFEXITED(status)) {
            printf("Exited current Process: %d\n", currentProcess + 1);
            fflush(stdout);
            alarm(quantum / 1000);
            currentProcess = (currentProcess + 1) % numProcesses;
            printf("Switched Processes to: %d\n", currentProcess + 1);
            fflush(stdout);
            kill(childPIDs[currentProcess], SIGCONT);
            if (processCheck[currentProcess] == 0) {
                processFinished++;
                processCheck[currentProcess] = 1;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    struct MethodCall *methods;

    //user protection
    //TODO: what if user didn't enter quantum
    //TODO: what if user didn't enter any arguments
    //TODO: what if user didn't enter any methods
    if (argc <= 2) {
        printf("Program needs method Name(s) and/or quantum in order to run.\n");
        return 0;
    }

    numberOfChildren(argc, argv);
    getQuantum(argv);
    methods = createMethods(numProcesses, argc, argv);

    roundRobinSchedule(methods);

    munmap(methods, sizeof(struct MethodCall) * numProcesses);

    return 0;
}
