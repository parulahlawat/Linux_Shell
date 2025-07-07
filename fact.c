#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <integer>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    if (n < 0) {
        fprintf(stderr, "Error: factorial of a negative number is not defined.\n");
        return 1;
    }

    long long factorial = 1;
    for (int i = 1; i <= n; i++) {
        factorial *= i;
    }

    printf("%lld\n", factorial);
    return 0;
}