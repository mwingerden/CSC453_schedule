#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>

#define MAX_PROCESSES 150
#define MAX_ARGUMENTS 10

//TODO: Memory check. Ex: check result in getMethods is null
//TODO: Test program where the user didn't input any :
//TODO: Test program with 100 methods

void two(int n) {
    if(n <= 0) {
        printf("Only natural numbers are accepted.\n");
        return;
    }
    int spaces = 8 * n;
    int i, j;
    for(i = 0; i < n; i++) {
        for(j = 0;j < spaces; j++) {
            printf(" ");
        }
        printf("%d\n", n);
    }
}

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
    int *quantum = mmap(NULL, sizeof(int), PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (quantum == MAP_FAILED) {
        perror("Failed mmap in getQuantum");
        EXIT_FAILURE;
    }
    *quantum = (int) strtol(argv[1], NULL, 10);
    return quantum;
}

char** getMethods(int argc, char* argv[]) {
    size_t totalSize = sizeof(char*) * (argc + 1); // +1 for the NULL pointer
    for (int i = 0; i < argc; i++) {
        totalSize += strlen(argv[i]) + 1; // +1 for null terminator
    }

    // Create a memory-mapped region to store argv
    char** result = mmap(NULL, totalSize, PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

    if (result == MAP_FAILED) {
        perror("Failed mmap in getMethods");
        EXIT_FAILURE;
    }

    // Copy argv and strings to the mmap region
    char* dataPtr = (char*)result + sizeof(char*) * (argc + 1); // Start after the array of pointers
    for (int i = 0; i < argc; i++) {
        result[i] = dataPtr; // Set the pointer in result to point to the copied string
        strcpy(dataPtr, argv[i]); // Copy the string
        dataPtr += strlen(argv[i]) + 1; // Move dataPtr to the next available position
    }

    result[argc] = NULL; // Null-terminate the array of pointers

    // Print the strings stored in the mmap region
//    for (int i = 0; result[i] != NULL; i++) {
//        printf("Argv[%d]: %s\n", i, result[i]);
//    }

    return result;
}

int main(int argc, char *argv[]) {
    int numChildren;
    int status;
    pid_t child_pid, wpid;
    int *quantum;
    char** methods;

    //user protection
    //TODO: what if user didn't enter quantum
    //TODO: what if user didn't enter any arguments
    //TODO: what if user didn't enter any methods
    if(argc <= 2) {
        printf("Program needs method(s) and/or quantum in order to run.\n");
        return 0;
    }

    // set variables
    numChildren = numberOfChildren(argc, argv);
    quantum = getQuantum(argv);
    methods = getMethods(argc, argv);

    // start forking
    for (int id=0; id<numChildren; id++) {
        if ((child_pid = fork()) == 0) {
            printf("Child %d\n", id + 1);
            exit(0);
        }
    }

    while ((wpid = wait(&status)) > 0);

    // deallocate memory
    munmap(quantum, sizeof(int));
    munmap(methods, argc * sizeof(char));

    return 0;
}
