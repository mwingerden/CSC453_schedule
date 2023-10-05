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
//    int quantum = mmap(NULL, sizeof(int), PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
//    if (quantum == MAP_FAILED) {
//        perror("Failed mmap in getQuantum\n");
//        exit(1);
//    }
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
        for (; methodCount < argc;) {
            strcpy(methods[structMethodCount].methodName, argv[methodCount]);
            methods[structMethodCount].arguments[0] = argv[methodCount];
            for (; argumentCount < argc; argumentCount++) {
                if (strcmp(argv[argumentCount], ":") != 0) {
                    methods[structMethodCount].arguments[structArgumentCount++] = argv[argumentCount];
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

    return methods;
}

void checkMethods(struct MethodCall *methods) {
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
}

void sigalrmHandler(int signum) {
// Stop the current process
    kill(childPIDs[currentProcess], SIGSTOP);
    printf("Entered Alarm with current process: %d\n", currentProcess + 1);
    fflush(stdout);

    // Move to the next process
    currentProcess = (currentProcess + 1) % numProcesses;
    printf("Switched Processes to: %d\n", currentProcess + 1);
    fflush(stdout);

    // Resume the next process
//    kill(childPIDs[currentProcess], SIGCONT);

    // Set the alarm for the next quantum
//    alarm(quantum / 1000);
}

void roundRobinSchedule(struct MethodCall *methods) {
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

    signal(SIGALRM, sigalrmHandler);

    currentProcess = 0;
    int processFinished = 0;
    int status;
    int processCheck[numProcesses];
    for (i = 0; i < numProcesses; i++) {
        processCheck[i] = 0;
    }
//    printf("Exited current Process: %d\n", currentProcess + 1);
//    fflush(stdout);
    alarm(quantum / 1000);
    kill(childPIDs[currentProcess], SIGCONT);
    while(processFinished < numProcesses) {
//        printf("Current Process: %d\n", currentProcess);
//        fflush(stdout);
        pid_t temp = waitpid(childPIDs[currentProcess], &status, WUNTRACED);

        if(temp > 0) {
//            alarm(quantum / 1000);
            if(WIFSTOPPED(status)) {
//                alarm(0);
//                printf("Stopped current Process: %d\n", currentProcess + 1);
//                fflush(stdout);
                alarm(quantum / 1000);
                kill(childPIDs[currentProcess], SIGCONT);
                printf("Started current Process: %d\n", currentProcess + 1);
                fflush(stdout);
            }
            else if(WIFEXITED(status)) {
                alarm(0);
                printf("Exited current Process: %d\n", currentProcess + 1);
                fflush(stdout);
                alarm(quantum / 1000);
                kill(childPIDs[currentProcess], SIGCONT);
                if(processCheck[currentProcess] == 0) {
                    processFinished++;
                    processCheck[currentProcess] = 1;
//                    currentProcess = (currentProcess + 1) % numProcesses;
//                    alarm(quantum / 1000);
                }
            }
//            currentProcess = (currentProcess + 1) % numProcesses;
        }
    }

//    kill(childPIDs[0], SIGCONT);
//
//    for (i = 0; i < numProcesses; i++) {
//        int status;
//        waitpid(childPIDs[i], &status, 0);
////        printf("Child %d terminated\n", i);
//    }
}

int main(int argc, char *argv[]) {
//    int numProcesses;
//    int quantum;
    struct MethodCall *methods;

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
    methods = createMethods(numProcesses, argc, argv);

//    checkMethods();

    roundRobinSchedule(methods);

    // Deallocate Memory
//    munmap(quantum, sizeof(int));
    munmap(methods, sizeof(struct MethodCall) * numProcesses);

    return 0;
}
