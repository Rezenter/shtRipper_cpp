#include "compress.h"
#include <fstream>

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
        bool bit = (buff[index / 8]&(1<< (index % 8))) != 0;
        index++;
        pGraph = &Graph[255];
        while(pGraph->Vertex[bit] > 255){
            pGraph = &Graph[pGraph->Vertex[bit] - 256];
            bit = (buff[index / 8]&(1<< (index % 8))) != 0;
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
            LongFlip flip{0};
            for(int i = 0; i < histogram->nPoints; i++){
                flip.asChar[0] = histogram->data[i];
                flip.asChar[1] = histogram->data[histogram->nPoints + i];
                flip.asChar[2] = histogram->data[histogram->nPoints * 2 + i];
                flip.asChar[3] = histogram->data[histogram->nPoints * 3 + i];

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

        out.point = new char[totalInSize * 15];
        currentOutPos = out.point;
        out.size = tasks.size();

        for(size_t i = 0; i < threadCount; i++){
            std::thread thread(worker);
            workers.push_back(move(thread));
        }

        for(std::thread &thread : workers){
            thread.join();
        }
        workers.clear();
    }
    return out;  // use currOutPos
}

CompressedRLE* compressRLE(const CombiscopeHistogram* uncompressed, const int size){
    auto* data = (unsigned char*) uncompressed;

    unsigned char counter;
    int i = 0;
    int NBytes = 0;
    while (i < size){
        if(i != size - 1 && data[i] == data[i+1]){
            counter = 2;
            i += 2;
            while((i < size) &&
                    (data[i] == data[i - 1]) &&
                    (counter < 127)){
                i++;
                counter++;
            }
            NBytes += 2;
        }else{
            counter=1;
            while (true){
                i++;
                if (i >= size)
                    break;
                if (i < size - 1){
                    if (data[i] == data[i + 1])
                        break;
                }
                if (counter == 127)
                    break;
                counter++;
            }
            NBytes += counter + 1;
        }
    }

    auto* compressed = new CompressedRLE(NBytes);

    i = 0;
    NBytes = 0;
    while (i < size){
        if(i != size - 1 && data[i] == data[i + 1]){
            counter = 2;
            compressed->data[NBytes + 1] = data[i];
            i += 2;
            while((i < size) &&
                    (data[i] == data[i - 1]) &&
                    (counter < 127)){
                i++;
                counter++;
            }
            compressed->data[NBytes] = counter;
            NBytes += 2;
        }else{
            counter = 1;
            compressed->data[NBytes + counter] = data[i];
            while(true){
                i++;
                if(i >= size)
                    break;
                if (i < size - 1){
                    if(data[i] == data[i + 1])
                        break;
                }
                if(counter == 127)
                    break;
                counter++;
                compressed->data[NBytes + counter] = data[i];
            }
            compressed->data[NBytes] = counter | 128;
            NBytes += counter + 1;
        }
    }

    return compressed;
}

void CreateCode(const unsigned char *table, Code *code){
    bool bits[256];
    unsigned short mask[256];
    for(unsigned short & entry : mask){
        entry = 512;
    }

    unsigned char vertex;
    unsigned short prevVertex;
    unsigned char bit, byteMask;
    unsigned int byte;

    for(int i = 0; i < 256; i++){
        code[i].NBits = 0;
        if (table[i] != 255){
            prevVertex = i;
            vertex=table[i];
            while (vertex != 255){
                if (mask[vertex] == 512){
                    mask[vertex] = prevVertex;
                }
                bits[code[i].NBits] = mask[vertex] != prevVertex;
                code[i].NBits++;
                prevVertex = vertex + 256;
                vertex = table[prevVertex];
            }

            for (int j = 0; j < code[i].NBits; j++){
                byte = (code[i].NBits - 1 - j) / 8;
                bit = (code[i].NBits - 1 - j) % 8;
                byteMask =~ (1 << bit);
                code[i].Bits[byte] = code[i].Bits[byte] & byteMask;
                code[i].Bits[byte] = code[i].Bits[byte] | (bits[j] << bit);
            }
        }
    }
}

int CompressedSize(const CompressedRLE* uncompressed, const Code *Code){
    int size = 0;
    for(int i = 0; i < uncompressed->size; i++){
        size += Code[uncompressed->data[i]].NBits;
    }
    return (size + 8) / 8;
}

void CompressHoffman(const CompressedRLE* uncompressed, const Code *code, unsigned char *outBuff){
    int index = 0;
    unsigned char bit, mask;
    unsigned int byte;
    for (int i = 0; i < uncompressed->size; i++){
        for (int j = 0; j < code[uncompressed->data[i]].NBits; j++){
            byte = j / 8;
            bit = j % 8;
            bool b = (code[uncompressed->data[i]].Bits[byte] & (1 << bit)) != 0;

            byte = index / 8;
            bit = index % 8;
            mask =~ (1 << bit);
            outBuff[byte] = outBuff[byte] & mask;
            outBuff[byte] = outBuff[byte] | (b << bit);

            index++;
        }
    }
}

void Sort(Knot **pKnot, const int left, const int right){
    int i, j;
    Knot *comp;
    Knot *value;
    i = left;
    j = right;
    comp = pKnot[(left + right) / 2];
    while (i <= j){
        while ((pKnot[i]->weight > comp->weight) && (i < right))
            i++;
        while ((pKnot[j]->weight) < comp->weight && (j > left))
            j--;
        if (i <= j){
            value = pKnot[i];
            pKnot[i] = pKnot[j];
            pKnot[j] = value;
            i++;
            j--;
        }
    }
    if(left < j)
        Sort(pKnot, left, j);
    if(i < right)
        Sort(pKnot, i, right);
}

void CreateTable(const CompressedRLE* uncompressed,	unsigned char *Table){
    int i;
    Knot knots[511], *pKnot[256];

    for(i = 0; i < uncompressed->size; i++){
        knots[uncompressed->data[i]].weight++;
    }

    int NKnots = 0;
    int NSymbols = 0;

    for(i = 0; i < 256; i++){
        if (knots[i].weight > 0){
            pKnot[NKnots] = &knots[i];
            NKnots++;
            NSymbols++;
        }
    }

    for(i = 0; i < NSymbols - 1; i++){
        Sort(pKnot,0,NKnots-1);
        knots[256 + i].weight = pKnot[NKnots - 2]->weight + pKnot[NKnots - 1]->weight;
        pKnot[NKnots - 2]->parent = i;
        pKnot[NKnots - 1]->parent = i;
        pKnot[NKnots - 2] = &knots[256 + i];
        NKnots--;
    }

    for(i = 0; i < 511; i++){
        Table[i] = (unsigned char)(knots[i].parent);
    }
}

CompressedHoff compressHoffman(const CompressedRLE* uncompressed){
    unsigned char table[511];

    Code code[256];
    CreateTable(uncompressed, table);
    CreateCode(table, code);
    int cSize = CompressedSize(uncompressed, code);

    auto *outBuff = new unsigned char [cSize + 511 + sizeof(int)];
    std::memcpy(outBuff, table, 511);
    std::memcpy(outBuff + 511, &uncompressed->size, sizeof(int));

    CompressHoffman(uncompressed, code, outBuff + 511 + sizeof(int));

    return CompressedHoff{
            (char*) outBuff,
            (cSize + 511 + 4)
    };
}

Out packSHT(const int signalCount, const char* headers, const char* data){
    std::cout << "pack" << std::endl;
    innerFreeOut();

    auto* currData = (long*)data;
    auto outQueue = new Out[signalCount];
    int totalSize = sizeof(V2) + sizeof(int);
    for(int signalIndex = 0; signalIndex < signalCount; signalIndex++){
        std::cout << "packing signal #" << signalIndex + 1 << std::endl;
        auto* raw_in = (CombiscopeHistogram *) (headers + sizeof(CombiscopeHistogram) * signalIndex);

        int flipSize = raw_in->nPoints;
        switch (raw_in->type>>16){
            case 0:
                break;
            case 1:
                flipSize *= 4; // 2 from long->double; 2 from x and y
                break;
            case 2:
                flipSize *= 6; // 2 from long->double; 3 from x, y and z
                std::cout << "Not implemented. Please, give this .sht file to Nikita" << std::endl;

                out.size = -1;
                return out;
            default:
                std::cout << "WTF? Not implemented. Please, give this .sht file to Nikita" << std::endl;

                out.size = -2;
                return out;
        }

        int signalSize = sizeof(CombiscopeHistogram) - sizeof(unsigned char *) + flipSize * sizeof(long);

        auto* buffer = new unsigned char[signalSize];
        std::memcpy(buffer, raw_in, sizeof(CombiscopeHistogram) - sizeof(unsigned char *));
        auto* buffPosition = buffer + sizeof(CombiscopeHistogram) - sizeof(unsigned char *);

        LongFlip flip{0};

        for(int i = 0; i < flipSize; i++){
            flip.asLong = currData[i];
            std::memcpy(buffPosition + i, &flip.asChar[0], sizeof(char));
            std::memcpy(buffPosition + i + flipSize, &flip.asChar[1], sizeof(char));
            std::memcpy(buffPosition + i + flipSize * 2, &flip.asChar[2], sizeof(char));
            std::memcpy(buffPosition + i + flipSize * 3, &flip.asChar[3], sizeof(char));
        }
        currData += flipSize;

        CompressedRLE* rle = compressRLE((CombiscopeHistogram*) buffer, signalSize);
        delete[] buffer;

        CompressedHoff packed = compressHoffman(rle);
        delete rle;

        totalSize += packed.size + sizeof(int);
        outQueue[signalIndex] = Out{
                packed.size,
                const_cast<char *>(packed.data)
        };
    }
    std::cout << "CPP pack OK" << std::endl;

    out.point = new char[totalSize];
    std::memcpy(out.point, V2, sizeof(V2));
    out.size += sizeof(V2);
    std::memcpy(out.point + out.size, &signalCount, sizeof(int));
    out.size += sizeof(int);
    for(int signalIndex = 0; signalIndex < signalCount; signalIndex++){
        std::memcpy(out.point + out.size, &outQueue[signalIndex].size, sizeof(int));
        out.size += sizeof(int);
        std::memcpy(out.point + out.size, outQueue[signalIndex].point, outQueue[signalIndex].size);
        out.size += outQueue[signalIndex].size;

        delete[] outQueue[signalIndex].point;
    }
    delete[] outQueue;

    std::cout << "CPP out OK" << std::endl;
    return out;
}

int innerTest(const int n){
    return n * n;
}

void innerFreeOut(){
    delete[] out.point;
    out.size = 0;
    currentOutPos = nullptr;
}