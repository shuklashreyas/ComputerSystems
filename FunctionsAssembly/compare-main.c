#include <stdio.h>
#include <stdlib.h>

extern long compare(long, long);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Two arguments required.\n");
        return 1;
    }

    long n1 = atol(argv[1]);
    long n2 = atol(argv[2]);

    long compareResult = compare(n1, n2);

    if (compareResult < 0) {
        printf("less\n");
    } else if (compareResult == 0) {
        printf("equal\n");
    } else {
        printf("greater\n");
    }

    return 0;
}





