#ifndef SHTRIPPER_COMPRESS_H
#define SHTRIPPER_COMPRESS_H
#include <windows.h>

int HistogramDataSize(int, int);

typedef char YNAME[16];
typedef struct tagHISTOGRAM
{
    int Type;
    char Name[128];
    char Comment[128];
    char Unit[128];
    SYSTEMTIME Time;  // windows.h
    int NChannels;
    double Tmin, Tmax;
    double Umin,Delta;
    LONG  Data[1];  // windows.h
}HISTOGRAM;
HISTOGRAM *InitHist(int,int);
HISTOGRAM *CreateHist(double *, int, int);

void* CompressRLE(void* , int, int *);
void* DecompressRLE(void* , int, int *);
void* CompressHoffman(void*, int, int *);
void* DecompressHoffman(void*, int, int *);
void ChangeByteOrder(void *, int, int, bool);


#endif //SHTRIPPER_COMPRESS_H
