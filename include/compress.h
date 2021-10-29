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
    int nPoints;
    double tMin, tMax;
    double yMin, delta;
    double*  data;  // windows.h
}CombiscopeHistogram;


typedef struct {
    unsigned short Vertex[2];
}CompressionGraph;
typedef struct {
    const char* data;
    const int size;
} CompressedHoff;
typedef struct {
    int size;
    char* point;
} Out;
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

static std::vector<std::thread> workers;
static std::vector<CompressedHoff> tasks;
static std::mutex lockIn;
static std::mutex lockOut;
static std::vector<CombiscopeHistogram*> results;
static Out out = {
        0,
        nullptr
};

static bool GetBit(const char* Bits, unsigned int index){
    return (Bits[index / 8]&(1<< (index % 8))) != 0;
}

void humanizeOut();

Out parseSHT(const char* in);

CompressedRLE* DecompressHoffman(const CompressedHoff* compressed);
CombiscopeHistogram* DecompressRLE(const CompressedRLE* compressed);

int innerTest(int n);

void innerFreeOut();

#endif //SHTRIPPER_COMPRESS_H
