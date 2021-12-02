#include "version.h"
#include "compress.h"

extern "C"{
    __declspec(dllexport) int test(int n) {return innerTest(n);}
    __declspec(dllexport) Out rip(const char* in, unsigned int rC, char* requests) {return parseSHT(in, rC, requests);}
    __declspec(dllexport) Out cram(const int count, const char* headers, const char* data) {return packSHT(count, headers, data);}
    //__declspec(dllexport) Out cramADC(const int count, const char* headers, const char* data) {return packADC(count, headers, data);}
    __declspec(dllexport) void freeOut() {return innerFreeOut();}
}