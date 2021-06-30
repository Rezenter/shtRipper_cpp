#include "compress.h"

#pragma warning(disable : 4996)
#include <windows.h>

int HistogramDataSize(int NChannels, int Type)
{

    switch (Type>>16)
    {
        case 0:
            return NChannels*sizeof(LONG);
        case 1:
            return NChannels*sizeof(double)*2;
        case 2:
            return NChannels*sizeof(double)*3;
        default:
            return NULL;
    }

}

HISTOGRAM *InitHist(int NChannels, int Type)
{
    HISTOGRAM *H;
    H=(HISTOGRAM*)GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,sizeof(HISTOGRAM)-sizeof(LONG)+
                                                       HistogramDataSize(NChannels, Type));
    if (!H) return NULL;
    H->NChannels=NChannels;
    H->Type=Type;
    GetLocalTime(&H->Time);
    FileSaved=FALSE;
    return H;
}

HISTOGRAM *CreateHist(double *F, int N, int Type)
{
    double Fmin,Fmax;
    int i;
    HISTOGRAM *H;
    H=InitHist(N,0);
    Fmin=F[0];
    Fmax=F[0];
    for (i=1;i<N;i++)
    {
        if (F[i]<Fmin)
            Fmin=F[i];
        if (F[i]>Fmax)
            Fmax=F[i];
    }
    if (Fmax==Fmin)
        Fmax=Fmin+1;

    for (i=0;i<N;i++)
        H->Data[i]=(int)Round((F[i]-Fmin)/(Fmax-Fmin)*0xffffffL);
    H->Delta=(Fmax-Fmin)/0xffffffL;
    H->Umin=Fmin;
    H->Type=Type;
    return H;
}



void* CompressRLE(void* inbuff, int SizeIn, int *SizeOut)
{
    int i;
    int NBytes;
#define InBuff ((unsigned char *)inbuff)
    unsigned char *OutBuff;
    unsigned char Counter;
    bool Repeat;


    i=0;
    NBytes=0;
    do
    {
        if (i==SizeIn-1)
        {
            Repeat=false;
        }
        else
        {
            Repeat=InBuff[i]==InBuff[i+1];
        }
        if (Repeat)
        {
            Counter=2;
            i+=2;
            while (
                    (i<SizeIn)&&
                    (InBuff[i]==InBuff[i-1])&&
                    (Counter<127))
            {
                i++;
                Counter++;
            }
            NBytes+=2;
        }
        else
        {
            Counter=1;
            do
            {
                i++;
                if (i>=SizeIn)
                    break;
                if (i<SizeIn-1)
                {
                    if (InBuff[i]==InBuff[i+1])
                        break;
                }
                if (Counter==127)
                    break;
                Counter++;
            }
            while (true);
            NBytes+=Counter+1;
        }
    }
    while (i<SizeIn);


    OutBuff=(unsigned char *)GlobalAlloc(GMEM_FIXED,NBytes);

    i=0;
    NBytes=0;
    do
    {
        if (i==SizeIn-1)
        {
            Repeat=false;
        }
        else
        {
            Repeat=InBuff[i]==InBuff[i+1];
        }
        if (Repeat)
        {
            Counter=2;
            OutBuff[NBytes+1]=InBuff[i];
            i+=2;
            while (
                    (i<SizeIn)&&
                    (InBuff[i]==InBuff[i-1])&&
                    (Counter<127))
            {
                i++;
                Counter++;
            }
            OutBuff[NBytes]=Counter;
            NBytes+=2;
        }
        else
        {
            Counter=1;
            OutBuff[NBytes+Counter]=InBuff[i];
            do
            {
                i++;
                if (i>=SizeIn)
                    break;
                if (i<SizeIn-1)
                {
                    if (InBuff[i]==InBuff[i+1])
                        break;
                }
                if (Counter==127)
                    break;
                Counter++;
                OutBuff[NBytes+Counter]=InBuff[i];
            }
            while (true);
            OutBuff[NBytes]=Counter|128;
            NBytes+=Counter+1;
        }
    }
    while (i<SizeIn);
    *SizeOut=NBytes;
    return (void *)OutBuff;
#undef InBuff
}

void* DecompressRLE(void* inbuff, int SizeIn, int *SizeOut)
{
    int i,j;
    int NBytes,Delta;
#define InBuff ((unsigned char *)inbuff)
    unsigned char *OutBuff;
    NBytes=0;
    i=0;
    do
    {
        Delta=InBuff[i]&127;
        if ((InBuff[i]&128)==0)
        {
            i+=2;
        }
        else
        {
            i+=Delta+1;
        }
        NBytes+=Delta;
    }
    while (i<SizeIn);
    OutBuff=(unsigned char *)GlobalAlloc(GMEM_FIXED,NBytes);
    NBytes=0;
    i=0;
    do
    {
        Delta=InBuff[i]&127;
        if ((InBuff[i]&128)==0)
        {
            for (j=0;j<Delta;j++)
            {
                OutBuff[NBytes+j]=InBuff[i+1];
            }
            i+=2;
        }
        else
        {
            for (j=0;j<Delta;j++)
            {
                OutBuff[NBytes+j]=InBuff[i+1+j];
            }
            i+=Delta+1;
        }
        NBytes+=Delta;
    }
    while (i<SizeIn);
    *SizeOut=NBytes;
    return (void *)OutBuff;
#undef InBuff
}

typedef struct tagKNOT
{
    int Weight;
    int Parent;
}KNOT;

void Sort(KNOT **pKnot, int left, int right)
{
    int i,j;
    KNOT *comp;
    KNOT *value;
    i=left;
    j=right;
    comp=pKnot[(left+right)/2];
    do
    {
        while ((pKnot[i]->Weight>comp->Weight) && (i < right))
            i++;
        while ((pKnot[j]->Weight)<comp->Weight && (j> left))
            j--;
        if (i<=j)
        {
            value=pKnot[i];
            pKnot[i]=pKnot[j];
            pKnot[j]=value;
            i++;
            j--;
        }
    }
    while (i<=j);
    if (left<j)
        Sort(pKnot,left,j);
    if (i<right)
        Sort(pKnot,i,right);
}


void CreateTable(unsigned char* InBuff, int SizeIn,	unsigned char *Table)
{
    int i;
    KNOT Knot[511], *pKnot[256];
    int NKnots,NSymbols;

    for (i=0;i<511;i++)
    {
        Knot[i].Parent=255;
        Knot[i].Weight=0;
    }
    for (i=0;i<SizeIn;i++)
    {
        Knot[InBuff[i]].Weight++;
    }
    NKnots=0;
    NSymbols=0;

    for (i=0;i<256;i++)
    {
        if (Knot[i].Weight>0)
        {
            pKnot[NKnots]=&Knot[i];
            NKnots++;
            NSymbols++;
        }
    }

    for (i=0;i<NSymbols-1;i++)
    {
        Sort(pKnot,0,NKnots-1);
        Knot[256+i].Weight=pKnot[NKnots-2]->Weight+pKnot[NKnots-1]->Weight;
        pKnot[NKnots-2]->Parent=i;
        pKnot[NKnots-1]->Parent=i;
        pKnot[NKnots-2]=&Knot[256+i];
        NKnots--;
    }
    for (i=0;i<511;i++)
    {
        Table[i]=(unsigned char)(Knot[i].Parent);
    }
}


typedef struct tagCODE
{
    unsigned char Bits[32];
    unsigned char NBits;
}CODE;
bool GetBit(unsigned char *Bits, unsigned int Index)
{
    unsigned char Bit;
    unsigned int Byte;
    Byte=Index/8;
    Bit=Index%8;
    return (Bits[Byte]&(1<<Bit))!=0;
}
typedef struct tagGRAPH
{
    unsigned short Vertex[2];
}GRAPH;

void SetBit(unsigned char *Bits, unsigned int Index, bool B)
{
    unsigned char Bit,Mask;
    unsigned int Byte;
    Byte=Index/8;
    Bit=Index%8;
    Mask=~(1<<Bit);
    Bits[Byte]=Bits[Byte]&Mask;
    Bits[Byte]=Bits[Byte]|(B<<Bit);
}


void CreateCode(unsigned char *Table, CODE *Code)
{
    int i,j;
    bool Bits[256];
    unsigned short Mask[256];
    unsigned char Vertex;
    unsigned short PrevVertex;

    for (i=0;i<256;i++)
    {
        Mask[i]=512;
    }
    for (i=0;i<256;i++)
    {
        Code[i].NBits=0;
        if (Table[i]!=255)
        {
            PrevVertex=i;
            Vertex=Table[i];
            do
            {
                if (Mask[Vertex]==512)
                {
                    Mask[Vertex]=PrevVertex;
                }
                Bits[Code[i].NBits]=Mask[Vertex]!=PrevVertex;
                Code[i].NBits++;
                PrevVertex=Vertex+256;
                Vertex=Table[PrevVertex];
            }
            while (Vertex!=255);
            for (j=0;j<Code[i].NBits;j++)
            {
                SetBit(Code[i].Bits,Code[i].NBits-1-j,Bits[j]);
            }
        }
    }
}

bool CreateGraph(unsigned char *Table, GRAPH *Graph)
{
    int i;
    unsigned short Mask[256];
    unsigned char Vertex;
    unsigned short PrevVertex;

    for (i=0;i<256;i++)
    {
        Mask[i]=512;
    }
    for (i=0;i<256;i++)
    {
        if (Table[i]!=255)
        {
            PrevVertex=i;
            Vertex=Table[i];
            do
            {
                if (Mask[Vertex]==512)
                {
                    Mask[Vertex]=PrevVertex;
                }
                Graph[Vertex].Vertex[Mask[Vertex]!=PrevVertex]=PrevVertex;
                PrevVertex=Vertex+256;
                Vertex=Table[PrevVertex];
                if (Vertex==PrevVertex-256)
                {
                    return false;
                }

            }
            while (Vertex!=255);
        }
    }
    Graph[255]=Graph[PrevVertex-256];
    return true;
}

int CompressedSize(unsigned char *InBuff, int SizeIn, CODE *Code)
{
    int i;
    int Size;
    Size=0;
    for (i=0;i<SizeIn;i++)
    {
        Size+=Code[InBuff[i]].NBits;
    }
    return (Size+8)/8;
}

void CompressHoffman(unsigned char *InBuff, int SizeIn, CODE *Code, unsigned char *OutBuff)
{
    int i,j;
    int Index;
    Index=0;
    for (i=0;i<SizeIn;i++)
    {
        for (j=0;j<Code[InBuff[i]].NBits;j++)
        {
            SetBit(OutBuff,Index,GetBit(Code[InBuff[i]].Bits,j));
            Index++;
        }
    }
}

void* CompressHoffman(void* inbuff, int SizeIn, int *SizeOut)
{
#define InBuff ((unsigned char *)inbuff)
    unsigned char Table[511];
    int cSize;
    unsigned char *OutBuff;
    CODE Code[256];
    CreateTable(InBuff,SizeIn,Table);
    CreateCode(Table,Code);
    cSize=CompressedSize(InBuff,SizeIn,Code);
    OutBuff=(unsigned char *)GlobalAlloc(GMEM_FIXED,cSize+511+4);
    memcpy(OutBuff,Table,511);
    memcpy(OutBuff+511,&SizeIn,4);
    CompressHoffman(InBuff,SizeIn,Code,OutBuff+511+4);
    *SizeOut=cSize+511+4;
    return OutBuff;
#undef InBuff
}

void *DecompressHoffman(void* inbuff, int SizeIn, int *SizeOut)
{
#define InBuff ((unsigned char *)inbuff)
    unsigned char *Table;
    unsigned char *OutBuff;
    int i,Index;
    GRAPH Graph[256],*pGraph;
    Table=InBuff;
    memcpy(SizeOut,InBuff+511,4);
    OutBuff=(unsigned char *)GlobalAlloc(GMEM_FIXED,*SizeOut);
    if (!CreateGraph(Table,Graph))
    {
        return NULL;
    }
    inbuff=(void *)(InBuff+511+4);
    Index=0;
    for (i=0;i<*SizeOut;i++)
    {
        bool B;
        B=GetBit(InBuff,Index);
        Index++;
        pGraph=&Graph[255];
        while (pGraph->Vertex[B]>255)
        {
            pGraph=&Graph[pGraph->Vertex[B]-256];
            B=GetBit(InBuff,Index);
            Index++;
        }
        OutBuff[i]=(unsigned char)pGraph->Vertex[B];
    }
    return OutBuff;
#undef InBuff
}


void ChangeByteOrder(void *data, int DataSize, int Level, bool Direction)
{
    char *Data1,*Data;
    int N0,N1;
    int i,n;
    Data=(char *)data;
    Data1=(char *)GlobalAlloc(GMEM_FIXED,DataSize);
    N0=0;
    for (n=0;n<Level;n++)
    {
        N1=DataSize/Level+((DataSize%Level)>n);
        if (Direction)
        {
            for (i=0;i<N1;i++)
            {
                Data1[N0+i]=Data[i*Level+n];
            }
        }
        else
        {
            for (i=0;i<N1;i++)
            {
                Data1[i*Level+n]=Data[N0+i];
            }
        }
        N0+=N1;
    }
    for (i=0;i<DataSize;i++)
    {
        Data[i]=Data1[i];
    }
    GlobalFree(Data1);
}

void *CompressHist(HISTOGRAM *H, int *Size)
{
    int Size0,Size1;
    HISTOGRAM* H0;
    void *H1,*H2;
    Size0=sizeof(HISTOGRAM)-sizeof(LONG)+HistogramDataSize(H->NChannels,H->Type);
    H0=(HISTOGRAM*)GlobalAlloc(GMEM_FIXED,Size0);
    memcpy(H0,H,Size0);
    ChangeByteOrder(&H0->Data[0],HistogramDataSize(H->NChannels,H->Type),4,true);
    H1=CompressRLE(H0,Size0,&Size1);
    H2=CompressHoffman(H1,Size1,Size);
    GlobalFree(H0);
    GlobalFree(H1);
    return H2;
}
HISTOGRAM *DecompressHist(void *H, int Size)
{
    int Size0,Size1;
    void *H1;
    HISTOGRAM *H2;
    H1=DecompressHoffman(H,Size,&Size1);
    if (H1==NULL)
    {
        return NULL;
    }
    H2=(HISTOGRAM *)DecompressRLE(H1,Size1,&Size0);
    GlobalFree(H1);
    ChangeByteOrder(&H2->Data[0],HistogramDataSize(H2->NChannels,H2->Type),4,false);
    return H2;
}
