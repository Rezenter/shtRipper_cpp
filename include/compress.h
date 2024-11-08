#ifndef SHTRIPPER_COMPRESS_H
#define SHTRIPPER_COMPRESS_H

#include <thread>
#include <mutex>
#include <vector>
#include <array>

#include <iostream>  // debug

static const char V0[] = "ANALIZER1.0";
static const char V1[] = "ANALIZER1.1";
static const char V2[] = "ANALIZER1.2";

int DefineVersion(const char * str);

typedef struct {
    unsigned short year;
    unsigned short month;
    unsigned short dayOfWeek;
    unsigned short day;
    unsigned short hour;
    unsigned short minute;
    unsigned short second;
    unsigned short milliseconds;
} Time;
typedef struct {
    int type;
    char name[128];
    char comment[128];
    char unit[128];
    Time time;
    int nPoints;
    double tMin, tMax, yMin, delta; // omitted in output
    unsigned char data[1];
}CombiscopeHistogram;

static const int SIGNAL_HEADER_SIZE = 408;

typedef struct {
    unsigned short Vertex[2];
}CompressionGraph;
typedef struct {
    unsigned char Bits[32];
    unsigned char NBits;
}Code;
class Knot{
public:
    int weight;
    int parent;

    Knot() : weight(0), parent(255){};
};

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

union LongFlip{
    long asLong;
    unsigned char asChar[4];
};

static std::vector<std::thread> workers;
static std::vector<CompressedHoff> tasks;
static std::mutex lockIn;
static std::mutex lockOut;
static unsigned int reqCount;
static char* req;
static Out out = {
        0,
        nullptr
};
static char* currentOutPos = nullptr;

Out parseSHT(const char* in, unsigned int reqCount, char* requests);
CompressedRLE* DecompressHoffman(const CompressedHoff* compressed);
CombiscopeHistogram* DecompressRLE(const CompressedRLE* compressed);
void appendOut(const CombiscopeHistogram*);

Out packSHT(int signalCount, const char* headers, const char* data);
Out packADC(int signalCount, const char* header, const char* data);
CompressedRLE* compressRLE(const CombiscopeHistogram* uncompressed, const int size);
CompressedHoff compressHoffman(const CompressedRLE* uncompressed);

static Out* outs;
//static size_t curr = 0;

int innerTest(int n);

void innerFreeOut();

#endif //SHTRIPPER_COMPRESS_H
