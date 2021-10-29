#include "version.h"
#include "compress.h"

extern "C"{
    __declspec(dllexport) int test(int n) {return innerTest(n);}
    __declspec(dllexport) Out rip(const char* in) {return parseSHT(in);}
    __declspec(dllexport) void freeOut() {return innerFreeOut();}
}