#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char *argv[]) {
    if(argc <= 2) {
        printf("Program needs method(s) and/or quantum in order to run.\n");
        return 0;
    }

    int quantum = (int) strtol(argv[1], NULL, 10);
    int numChildren = numberOfChildren(argc, argv);
    printf("Quantum: %d, Number of Children: %d", quantum, numChildren);

    return 0;
}
