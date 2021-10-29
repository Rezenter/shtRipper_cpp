#include "compress.h"

int DefineVersion(const char * str){
    if (strcmp(str, V0) == 0){
        return 0;
    }
    else if (strcmp(str, V1) == 0){
        return 1;
    }
    else if (strcmp(str, V2) == 0){
        return 2;
    }
    return -1;
}

CombiscopeHistogram* DecompressRLE(const CompressedRLE* compressed){
    int j;
    int Delta;
    int NBytes = 0;
    int i = 0;
    while(i < compressed->size){
        Delta = compressed->data[i]&127;
        if ((compressed->data[i]&128) == 0){
            i += 2;
        }else{
            i += Delta + 1;
        }
        NBytes += Delta;
    }
    auto OutBuff = new unsigned char[NBytes];

    NBytes = 0;
    i = 0;

    while(i < compressed->size){
        Delta=compressed->data[i]&127;
        if ((compressed->data[i]&128) == 0){
            for (j = 0; j < Delta; j++){
                OutBuff[NBytes + j] = compressed->data[i + 1];
            }
            i += 2;
        }else{
            for(j = 0; j < Delta; j++){
                OutBuff[NBytes + j] = compressed->data[i + 1 + j];
            }
            i += Delta + 1;
        }
        NBytes += Delta;
    }

    return (CombiscopeHistogram*)OutBuff;
}

bool CreateGraph(const CompressedHoff* signal, CompressionGraph *Graph){
    unsigned short mask[256];
    for(unsigned short & el : mask){
        el = 512;
    }

    unsigned char Vertex;
    unsigned short PrevVertex;
    for(int i = 0; i < 256; i++){
        if (*(unsigned char*)(&signal->data[i]) != 255){
            PrevVertex = i;
            Vertex = signal->data[i];
            do{
                if(mask[Vertex] == 512){
                    mask[Vertex] = PrevVertex;
                }
                Graph[Vertex].Vertex[mask[Vertex] != PrevVertex] = PrevVertex;
                PrevVertex = Vertex + 256;
                Vertex = signal->data[PrevVertex];
                if (Vertex == PrevVertex - 256) {
                    return false;
                }
            }while(Vertex != 255);
        }
    }
    Graph[255] = Graph[PrevVertex - 256];
    return true;
}

CompressedRLE* DecompressHoffman(const CompressedHoff* compressed){
    CompressionGraph Graph[256], *pGraph;
    if(!CreateGraph(compressed, Graph)){
        return nullptr;
    }

    int uncompressedSize;
    memcpy(&uncompressedSize, compressed->data + 511, sizeof(int));
    auto RLE = new CompressedRLE(uncompressedSize);

    int index = 0;
    const auto* buff = compressed->data + 511 + sizeof(int);

    for(int i = 0; i < uncompressedSize; i++){
        bool bit = GetBit(buff, index);
        index++;
        pGraph = &Graph[255];
        while(pGraph->Vertex[bit] > 255){
            pGraph = &Graph[pGraph->Vertex[bit] - 256];
            bit = GetBit(buff, index);
            index++;
        }
        RLE->data[i] = (unsigned char)pGraph->Vertex[bit];
    }
    return RLE;
}

CombiscopeHistogram *DecompressHist(const CompressedHoff compressed){
    CompressedRLE* signalRLE = DecompressHoffman(&compressed);
    if(signalRLE == nullptr){
        return nullptr;
    }

    auto *signal = DecompressRLE(signalRLE);
    delete signalRLE;

    //ChangeByteOrder(&signal->Data[0], HistogramDataSize(signal->NChannels, signal->Type));
    return signal;
}

void worker(){
    CombiscopeHistogram* signal;
    while(true) {
        lockIn.lock();
        if (tasks.empty()) {
            lockIn.unlock();
            return;
        }
        CompressedHoff task = tasks.back();
        tasks.pop_back();
        lockIn.unlock();

        signal = DecompressHist(task);
        if(signal != nullptr){
            lockOut.lock();
            results.push_back(signal);
            lockOut.unlock();
        }
    }
}

int histogramSize(int nPoints, int Type){
    switch (Type>>16)
    {
        case 0:
            return nPoints * sizeof(LONG) + sizeof(CombiscopeHistogram) - sizeof(LONG);
        case 1:
            return nPoints * sizeof(double) * 2 + sizeof(CombiscopeHistogram) - sizeof(LONG);
        case 2:
            return nPoints * sizeof(double) * 3 + sizeof(CombiscopeHistogram) - sizeof(LONG);
        default:
            return 0;
    }

}

double histogramTmin(CombiscopeHistogram *H){
    switch (H->type>>16){
        case 0:
            return H->tMin;
        case 1:
            //return ((double *)(&H->Data[0]))[0];
        case 2:
            return ((double *)(&H->data[0]))[0];
        default:
            return 0;
    }
}
double histogramTmax(CombiscopeHistogram *H){
    switch (H->type>>16){
        case 0:
            return H->tMax;
        case 1:
            return ((double *)(&H->data[0]))[(H->nPoints - 1) * 2];
        case 2:
            return ((double *)(&H->data[0]))[(H->nPoints - 1) * 3];
        default:
            return 0;
    }
}
double HistogramXValue(CombiscopeHistogram *H, int i){
    switch (H->type>>16){
        case 0:
            return (double)i * (H->tMax - H->tMin) / (H->nPoints - 1) + H->tMin;
        case 1:
            return ((double *)(&H->data[0]))[i * 2];
        case 2:
            return ((double *)(&H->data[0]))[i * 3];
        default:
            return 0;
    }
}
double HistogramYValue(CombiscopeHistogram *H, int i){
    switch (H->type>>16)
    {
        case 0:
            return H->data[i] * H->delta + H->yMin;
        case 1:
            return ((double *)(&H->data[0]))[i * 2 + 1];
        case 2:
            return ((double *)(&H->data[0]))[i * 3 + 1];
        default:
            return 0;
    }
}
double HistogramEValue(CombiscopeHistogram *H, int i){
    switch (H->type>>16){
        case 2:
            return ((double *)(&H->data[0]))[i * 3 + 2];
        default:
            return 0;
    }
}

void humanizeOut(){
    for(CombiscopeHistogram* &signal : results){
        out.size += histogramSize(signal->nPoints, signal->type);
    }
    if(out.size > 0){
        out.point = new char[out.size];
        char* curr = out.point;
        for(CombiscopeHistogram* &signal : results){
            size_t curr_size = histogramSize(signal->nPoints, signal->type);
            memcpy(curr, signal, curr_size);
            curr += curr_size;
            delete signal;
        }
    }
    results.clear();
}

Out parseSHT(const char* in) { // adapted version of RestoreHist(..., int version)
    int signalSize;
    innerFreeOut();

    const int version = DefineVersion(in);
    size_t currentPos = sizeof(V1);
    if(version == -1){
        out.size = -1;
        return out;
    }

    int signalCount;
    memcpy(&signalCount, in + currentPos, sizeof(int));
    currentPos += sizeof(int);

    for(int signalIndex = 0; signalIndex < signalCount; signalIndex++){
        switch(version){
            case 0: {
                out.size = -2;
                return out;
            }
            case 1:{
                out.size = -3;
                return out;
            }
            case 2:{
                memcpy(&signalSize, in + currentPos, sizeof(int));
                currentPos += sizeof(int);

                if(signalSize <= 0){
                    out.size = -4;
                    return out;
                }
                tasks.push_back(CompressedHoff {
                        in + currentPos,
                        signalSize
                    }
                );

                currentPos += signalSize;
                break;
            }
            default: {
                out.size = -5;
                return out;
            }
        }
    }
    if(!tasks.empty()){
        size_t threadCount = std::thread::hardware_concurrency();

        for(size_t i = 0; i < threadCount; i++){
            std::thread thread(worker);
            workers.push_back(move(thread));
        }

        for(std::thread &thread : workers){
            thread.join();
        }
        workers.clear();

        humanizeOut();
    }
    return out;
}

int innerTest(const int n){
    return n * n;
}

void innerFreeOut(){
    delete out.point;
    out.size = 0;
}