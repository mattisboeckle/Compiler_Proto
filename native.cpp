#include <cstdio>
#include <iostream>

extern "C"
int printi(int val) {
    printf("%d\n", val);
    return 0;
}

extern "C"
int printd(double val) {
    printf("%f\n", val);
    return 0;
}

extern "C"
int print(char* str) {
    printf("%s\n", str);
    return 0;
}