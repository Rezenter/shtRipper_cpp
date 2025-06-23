#include "compress.h"

#if defined(_MSC_VER)
    //  Microsoft
    #define EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
    //  GCC
    #define EXPORT __attribute__((visibility("default")))
#else
    #pragma error Unknown dynamic link import/export semantics.
#endif


extern "C"{
    EXPORT int test(int n) {return innerTest(n);}
    EXPORT Out rip(const char* path, unsigned int rC, char* requests) {return parseSHT(path, rC, requests);}
    EXPORT Out cram(const int count, const char* headers, const char* data) {return packSHT(count, headers, data);}
    EXPORT Out cramADC(const int count, const char* headers, const char* data) {return packADC(count, headers, data);}
    EXPORT void freeOut() {return innerFreeOut();}
}