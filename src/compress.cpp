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
    std::memcpy(&uncompressedSize, compressed->data + 511, sizeof(int));
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
            appendOut(signal);
        }
    }
}

void appendOut(const CombiscopeHistogram* histogram){
    lockOut.lock();
    std::memcpy(currentOutPos, histogram, SIGNAL_HEADER_SIZE);
    currentOutPos += SIGNAL_HEADER_SIZE;
    auto* dBuff = (double*)currentOutPos;

    int arrSize = histogram->nPoints * sizeof(double);
    switch (histogram->type>>16){
        case 0:
            std::memcpy(currentOutPos, &histogram->tMin, 2 * sizeof(double));
            dBuff = (double*)(currentOutPos + 2 * sizeof(double));
            currentOutPos += arrSize + 2 * sizeof(double);
            break;
        case 1:
            currentOutPos += 2 * arrSize;
            break;
        case 2:
            currentOutPos += 3 * arrSize;
            break;
        default:
            break;
    }
    lockOut.unlock();

    switch (histogram->type>>16){
        case 0:{
            auto* cData = (unsigned char*)&histogram->data;
            LongFlip flip{0};
            for(int i = 0; i < histogram->nPoints; i++){
                flip.asChar[0] = cData[i];
                flip.asChar[1] = cData[histogram->nPoints + i];
                flip.asChar[2] = cData[histogram->nPoints * 2 + i];
                flip.asChar[3] = cData[histogram->nPoints * 3 + i];

                dBuff[i] = flip.asLong * histogram->delta + histogram->yMin; // y coordinates
            }
            break;
        }
        case 1:
            std::cout << "Not implemented. Please, give this .sht file to Nikita" << std::endl;
            break;
        case 2:
            std::cout << "Not implemented. Please, give this .sht file to Nikita" << std::endl;
            break;
        default:
            break;
    }

    delete histogram;
}

Out parseSHT(const char* in) { // adapted version of RestoreHist(..., int version)
    int compressedSignalSize;
    innerFreeOut();

    const int version = DefineVersion(in);
    size_t currentPos = sizeof(V1);
    if(version == -1){
        out.size = -1;
        return out;
    }

    int signalCount;
    std::memcpy(&signalCount, in + currentPos, sizeof(int));
    currentPos += sizeof(int);

    int totalInSize = 0;
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
                std::memcpy(&compressedSignalSize, in + currentPos, sizeof(int));
                totalInSize += compressedSignalSize;
                currentPos += sizeof(int);

                if(compressedSignalSize <= 0){
                    out.size = -4;
                    return out;
                }
                tasks.push_back(CompressedHoff {
                        in + currentPos,
                        compressedSignalSize
                    }
                );

                currentPos += compressedSignalSize;
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

        int mem = totalInSize * 15;
        out.point = new unsigned char[mem];
        currentOutPos = out.point;


        for(size_t i = 0; i < threadCount; i++){
            std::thread thread(worker);
            workers.push_back(move(thread));
        }

        for(std::thread &thread : workers){
            thread.join();
        }
        workers.clear();
    }
    out.size = currentOutPos - out.point;
    return out;  // use currOutPos
}

int innerTest(const int n){
    return n * n;
}

void innerFreeOut(){
    delete out.point;
    out.size = 0;
    currentOutPos = nullptr;
}