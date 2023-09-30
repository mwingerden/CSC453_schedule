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
    char *arguments[MAX_ARGUMENTS];
};


//void two(int n) {
//    if(n <= 0) {
//        printf("Only natural numbers are accepted.\n");
//        return;
//    }
//    int spaces = 8 * n;
//    int i, j;
//    for(i = 0; i < n; i++) {
//        for(j = 0;j < spaces; j++) {
//            printf(" ");
//        }
//        printf("%d\n", n);
//    }
//}

int numberOfChildren(int argc, char *argv[]) {
    int semicolonCount = 1;
    int i = 0;

    for(; i < argc; i++) {
        if(!strcmp(argv[i], ":")) {
            semicolonCount++;
        }
    }

    return semicolonCount;
}

int* getQuantum(char *argv[]) {
    int *quantum = mmap(NULL, sizeof(int), PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (quantum == MAP_FAILED) {
        perror("Failed mmap in getQuantum\n");
        exit(1);
    }
    *quantum = (int) strtol(argv[1], NULL, 10);
    return quantum;
}

struct MethodCall* getMethods(int numMethods, int argc, char *argv[]) {
    //TODO: Handle over 150 process calls.
    //TODO: Handle over 10 argument calls.
    size_t totalSize = sizeof(struct MethodCall) * numMethods;

    struct MethodCall *mappedMethods = (struct MethodCall *)mmap(
            NULL,
            totalSize,
            PROT_WRITE, // Set the appropriate permissions
            MAP_SHARED | MAP_ANONYMOUS,
            -1,
            0
    );

    if (mappedMethods == MAP_FAILED) {
        perror("Failed mmap in getMethods\n");
        exit(1);
    }

    int structMethodCount = 0;
    int structArgumentCount = 0;
    int methodCount = 2;
    int argumentCount = 3;
    for(; structMethodCount < numMethods; structMethodCount++) {
        for(; methodCount < argc;) {
            strcpy(mappedMethods[structMethodCount].methodName, argv[methodCount]);
            for(; argumentCount < argc; argumentCount++) {
                if(strcmp(argv[argumentCount], ":") != 0) {
                    mappedMethods[structMethodCount].arguments[structArgumentCount++] = argv[argumentCount];
                }
                else {
                    methodCount += 2 + structArgumentCount;
                    argumentCount += 2;
                    structArgumentCount = 0;
                    break;
                }
            }
            break;
        }
    }

    return mappedMethods;
}

void handeSignals(int signum) {
//    if (signum == SIGCONT) {
//        printf("Received SIGCONT\n");
//        // Handle continued execution
//    } else if (signum == SIGSTOP) {
//        printf("Received SIGSTOP\n");
//        // Handle paused execution
//    }
}

void childProcess(int id) {
    int i = 0;
    for(; i < 100; i++) {
        printf("Child Process %d: number %d\n", id, i + 1);
        fflush(stdout);
        sleep(1);
    }
}

void parentProcess(int numChildren, pid_t childPIDs[], const int *quantum) {
    // Parent Process
    pid_t wpid;
    int numFinished = 0;
    int currentChild = 0;
    int childFinished[numChildren];

    int i;
    for(i = 0; i < numChildren; i++) {
        childFinished[i] = 0;
    }

    while(numFinished < numChildren) {
//        printf("parent starts\n");
        kill(childPIDs[currentChild], SIGCONT);
        sleep(*quantum / 1000);
        kill(childPIDs[currentChild], SIGSTOP);
//        waitpid(childPIDs[currentChild], NULL, WUNTRACED);
        wpid = waitpid(childPIDs[currentChild], NULL, WNOHANG);

        if(wpid > 0) {
            // Child has finished
            printf("Parent: Child exited\n");
            numFinished++;
        }
        else {
        }
        currentChild++;
        if(currentChild >= numChildren) {
            currentChild = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    int numChildren;
    int *quantum;
    struct MethodCall* methods;

    //user protection
    //TODO: what if user didn't enter quantum
    //TODO: what if user didn't enter any arguments
    //TODO: what if user didn't enter any methods
    if(argc <= 2) {
        printf("Program needs method Name(s) and/or quantum in order to run.\n");
        return 0;
    }

    // set variables
    numChildren = numberOfChildren(argc, argv);
    quantum = getQuantum(argv);
    methods = getMethods(numChildren, argc, argv);

//    int count;
//    int count1;
//    for(count = 0; count < numChildren; count++) {
//        printf("methodName call %d:\n\tName: %s\n", count+1, methods[count].methodName);
//        for(count1 = 0; count1 < MAX_ARGUMENTS; count1++) {
//            if(methods[count].arguments[count1] == NULL) {
//                break;
//            }
//            printf("\targument %d: %s\n", count1 + 1, methods[count].arguments[count1]);
//        }
//    }

    // start forking
    pid_t childPIDs[numChildren];
    for (int id=0; id<numChildren; id++) {
        pid_t childPID = fork();
        if (childPID == 0) {
            signal(SIGCONT, handeSignals);
            signal(SIGSTOP, handeSignals);
            // Child is initially paused
            printf("Child process (PID %d) is waiting...\n", getpid());

            // The child will pause here until it receives SIGCONT from the parent
            pause();

            //Child execution
            childProcess(id);

            exit(0);
        }
        else if (childPID > 0){
            childPIDs[id] = childPID;
        }
        else {
            perror("Fork Failed");
            return 1;
        }
    }
//
//    // Parent Process
//    parentProcess(numChildren, childPIDs, quantum);

    // Deallocate Memory
    munmap(quantum, sizeof(int));
    munmap(methods, sizeof(struct MethodCall) * numChildren);

    return 0;
}
