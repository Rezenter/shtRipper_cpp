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

int32_t DefineVersion(const char * str);

typedef struct {
    uint16_t year;
    uint16_t month;
    uint16_t dayOfWeek;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
    uint16_t milliseconds;
} Time;
typedef struct {
    int32_t type;
    char name[128];
    char comment[128];
    char unit[128];
    Time time;
    int32_t nPoints;
    double tMin, tMax, yMin, delta; // omitted in output
    unsigned char data[1];
}CombiscopeHistogram;

static const int32_t SIGNAL_HEADER_SIZE = 408;

typedef struct {
    uint16_t Vertex[2];
}CompressionGraph;
typedef struct {
    unsigned char Bits[32];
    unsigned char NBits;
}Code;
class Knot{
public:
    int32_t weight;
    int32_t parent;

    Knot() : weight(0), parent(255){};
};

typedef struct {
    const char* data;
    const size_t size;
} CompressedHoff;
typedef struct {
    size_t size;
    char* point;
} Out;
class CompressedRLE{
public:
    unsigned char* data;
    const size_t size;

    explicit CompressedRLE(const int32_t size) : size(size){
        data = new unsigned char [size];
    };
    ~CompressedRLE(){
        delete[] data;
    }
};

union LongFlip{
    int32_t asLong;
    unsigned char asChar[4];
};
union LongToDouble {
    int32_t asLong[2];
    double asDouble;
};

static std::vector<std::thread> workers;
static std::vector<CompressedHoff> tasks;
static std::mutex lockIn;
static std::mutex lockOut;
static uint32_t reqCount;
static char* req;
static Out out = {
        0,
        nullptr
};
static char* currentOutPos = nullptr;

Out parseSHT(const char* path, uint32_t reqCount, char* requests);

CompressedRLE* DecompressHoffman(const CompressedHoff* compressed);
CombiscopeHistogram* DecompressRLE(const CompressedRLE* compressed);
void appendOut(const CombiscopeHistogram*);

Out packSHT(int32_t signalCount, const char* headers, const char* data);
Out packADC(int32_t signalCount, const char* headers, const char* data);
CompressedRLE* compressRLE(const CombiscopeHistogram* uncompressed, const int32_t size);
CompressedHoff compressHoffman(const CompressedRLE* uncompressed);

static Out* outs;
//static size_t curr = 0;

int32_t innerTest(int32_t n);

void innerFreeOut();

#endif //SHTRIPPER_COMPRESS_H
