#include "compress.h"

std::vector<std::thread> workers;
std::vector<CompressedHoff> tasks;
std::mutex lockIn;
std::mutex lockOut;
std::vector<HISTOGRAM*> results;

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

HISTOGRAM* DecompressRLE(const CompressedRLE* compressed){
    int j;
    int Delta;
    int NBytes = 0;
    int i = 0;
    while(i < compressed->size){
        Delta = compressed->data[i]&127;
        if ((compressed->data[i]&128)==0){
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
        if ((compressed->data[i]&128)==0){
            for (j=0;j<Delta;j++){
                OutBuff[NBytes+j]=compressed->data[i+1];
            }
            i+=2;
        }else{
            for (j=0;j<Delta;j++){
                OutBuff[NBytes+j]=compressed->data[i+1+j];
            }
            i+=Delta+1;
        }
        NBytes+=Delta;
    }

    return (HISTOGRAM*)OutBuff;
}

bool GetBit(const char* Bits, unsigned int index){
    unsigned char Bit = index % 8;
    unsigned int Byte = index / 8;
    return (Bits[Byte]&(1<<Bit)) != 0;
}

bool CreateGraph(const CompressedHoff* signal, GRAPH *Graph){
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
    GRAPH Graph[256], *pGraph;
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

HISTOGRAM *DecompressHist(const CompressedHoff compressed){
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
    HISTOGRAM* signal;
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

bool parseSHT(const char* buffer) { // adapted version of RestoreHist(..., int version)
    int signalSize;

    const int version = DefineVersion(buffer);
    size_t currentPos = sizeof(V1);
    if(version == -1){
        return false;
    }

    int signalCount;
    memcpy(&signalCount, buffer + currentPos, sizeof(int));
    currentPos += sizeof(int);

    for(int signalIndex = 0; signalIndex < signalCount; signalIndex++){
        switch(version){
            case 0: {
                std::cout << "sht v0 is NOT IMPLEMENTED! please, contact Zhiltsov Nikita" << std::endl;
                break;
            }
            case 1:{
                std::cout << "sht v1 is NOT IMPLEMENTED! please, contact Zhiltsov Nikita" << std::endl;
                break;
            }
            case 2:{
                memcpy(&signalSize, buffer + currentPos, sizeof(int));
                currentPos += sizeof(int);

                if(signalSize <= 0){
                    return false;
                }
                tasks.push_back(CompressedHoff {
                        buffer + currentPos,
                        signalSize
                    }
                );

                currentPos += signalSize;
                break;
            }
            default: {
                std::cout << "WTF? " << version << std::endl;
                return false;
            }
        }
    }
    if(!tasks.empty()){
        size_t threadCount = std::min((size_t)std::thread::hardware_concurrency(), tasks.size());
        for(size_t i = 0; i < threadCount; i++){
            std::thread thread(worker);
            workers.push_back(move(thread));
        }

        for(std::thread &thread : workers){
            thread.join();
        }

        for(HISTOGRAM* &signal : results){
            //std::cout << "signal: " << signal->name << ", " << signal->unit << std::endl;
            delete signal;
        }
    }
    return true;
}