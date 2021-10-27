#ifndef SHTRIPPER_COMPRESS_H
#define SHTRIPPER_COMPRESS_H
#include <windows.h>
#include <cmath>
#include <thread>
#include <mutex>
#include <vector>

#include <iostream>  // debug

static const char V0[] = "ANALIZER1.0";
static const char V1[] = "ANALIZER1.1";
static const char V2[] = "ANALIZER1.2";

int DefineVersion(const char * str);

typedef struct {
    int type;
    char name[128];
    char comment[128];
    char unit[128];
    SYSTEMTIME time;  // windows.h
    int chCount;
    double tMin, tMax;
    double yMin, delta;
    LONG  data[];  // windows.h
}HISTOGRAM;

typedef struct {
    unsigned short Vertex[2];
}GRAPH;
typedef struct {
    const char* data;
    const int size;
} CompressedHoff;
class CompressedRLE{
public:
    unsigned char* data;
    const int size;

    explicit CompressedRLE(const int size) : size(size){
        data = new unsigned char [size];
    };
    ~CompressedRLE(){
        delete[] data;
    }
};


bool parseSHT(const char * buffer);

CompressedRLE* DecompressHoffman(const CompressedHoff* compressed);
HISTOGRAM* DecompressRLE(const CompressedRLE* compressed);

#endif //SHTRIPPER_COMPRESS_H
