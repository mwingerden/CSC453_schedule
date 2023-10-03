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
int current_process = 0;
pid_t childPIDs[MAX_PROCESSES];
struct MethodCall *methods;

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
//    int quantum = mmap(NULL, sizeof(int), PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
//    if (quantum == MAP_FAILED) {
//        perror("Failed mmap in getQuantum\n");
//        exit(1);
//    }
    quantum = (int) strtol(argv[1], NULL, 10);
}

void getMethods(int numMethods, int argc, char *argv[]) {
    //TODO: Handle over 150 process calls.
    //TODO: Handle over 10 argument calls.
    size_t totalSize = sizeof(struct MethodCall) * numMethods;

    methods = (struct MethodCall *) mmap(
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

    int i;
    int j;
    for(i = 0; i < numProcesses; i++) {
        for(j = 0; j < MAX_ARGUMENTS + 1; j++) {
            methods[i].arguments[j] = NULL;
        }
    }

    char methodName[50] = "";
    int structMethodCount = 0;
    int structArgumentCount = 1;
    int methodCount = 2;
    int argumentCount = 3;
    for (; structMethodCount < numMethods; structMethodCount++) {
        for (; methodCount < argc;) {
//            strcat(methodName, argv[methodCount]);
            strcpy(methods[structMethodCount].methodName, argv[methodCount]);
            methods[structMethodCount].arguments[0] = argv[methodCount];
//            strcat(mappedMethods[structMethodCount].methodName, ".c");
            for (; argumentCount < argc; argumentCount++) {
                if (strcmp(argv[argumentCount], ":") != 0) {
                    methods[structMethodCount].arguments[structArgumentCount++] = argv[argumentCount];
                } else {
                    methods[numProcesses].arguments[structArgumentCount] = NULL;
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

void checkMethods() {
    int count;
    int count1;
    for (count = 0; count < numProcesses; count++) {
        printf("methodName call %d:\n\tName: %s\n", count + 1, methods[count].methodName);
        for (count1 = 0; count1 < MAX_ARGUMENTS + 1; count1++) {
            if (methods[count].arguments[count1] == NULL) {
                break;
            }
            printf("\targument %d: %s\n", count1 + 1, methods[count].arguments[count1]);
        }
    }
}

void signalHandler(int signal) {
//    if (signal == SIGCONT) {
//        printf("Child %d received SIGCONT signal from parent.\n", getpid());
//    }
//    else if (signal == SIGSTOP) {
//        printf("Child %d received SIGSTOP signal from parent.\n", getpid());
//    }
}

void sigalrm_handler(int signum) {
//    printf("Reached the alarm\n");
    // Stop current process
    kill(childPIDs[current_process], SIGSTOP);
    // Switch to the next process
    current_process = (current_process + 1) % numProcesses;
    // Send a SIGUSR1 signal to the next process to wake it up
    kill(childPIDs[current_process], SIGCONT);
    alarm(quantum / 1000); // Set the alarm for the next quantum
}

void roundRobinSchedule() {
    int i;

//    checkMethods(numProcesses, methods);

    for (i = 0; i < numProcesses; i++) {
        childPIDs[i] = fork();

        if (childPIDs[i] == -1) {
            perror("Fork failed");
            exit(1);
        }

        if (childPIDs[i] == 0) {
            // This is a child process
            signal(SIGCONT, signalHandler);

//            printf("Child %d is waiting for a signal from the parent...\n", getpid());
            pause(); // Wait for a signal to be received
//            printf("Child %d continued...\n", getpid());
//            sleep(2);
//            printf("Method name: %s\n", methods[i].methodName);
            if (execvp(methods[i].methodName, methods[i].arguments) == -1) {
                perror("execvp");
                fprintf(stderr, "Failed to execute: %s\n", methods[i].methodName);
                exit(EXIT_FAILURE);
            }

            exit(0); // Exit the child process after receiving the signal
        }
    }

    // This is the parent process
    sleep(1); // Give the children some time to set up their signal handlers

    signal(SIGALRM, sigalrm_handler);
    alarm(quantum / 1000);

    kill(childPIDs[0], SIGCONT);

    for (i = 0; i < numProcesses; i++) {
        int status;
        waitpid(childPIDs[i], &status, 0);
//        printf("Child %d terminated\n", i);
    }
}

int main(int argc, char *argv[]) {
//    int numProcesses;
//    int quantum;

    //user protection
    //TODO: what if user didn't enter quantum
    //TODO: what if user didn't enter any arguments
    //TODO: what if user didn't enter any methods
    if (argc <= 2) {
        printf("Program needs method Name(s) and/or quantum in order to run.\n");
        return 0;
    }

    // set variables
    numberOfChildren(argc, argv);
    getQuantum(argv);
    getMethods(numProcesses, argc, argv);

//    char cwd[1024];
//    if (getcwd(cwd, sizeof(cwd)) != NULL) {
//        printf("Current working directory: %s\n", cwd);
//    } else {
//        perror("getcwd");
//    }
    checkMethods();

//    roundRobinSchedule();

    // Deallocate Memory
//    munmap(quantum, sizeof(int));
    munmap(methods, sizeof(struct MethodCall) * numProcesses);

    return 0;
}
