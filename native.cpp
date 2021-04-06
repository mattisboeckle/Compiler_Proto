#include <cstdio>
#include <iostream>

extern "C"
void printi(int val) {
    printf("%d\n", val);
}

extern "C"
void printd(double val) {
    printf("%f\n", val);
}

extern "C"
void print(char* str) {
    printf("%s\n", str);
}