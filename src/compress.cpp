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

        out.point = new unsigned char[totalInSize * 15];
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

void packSHT(){
    std::cout << "pack" << std::endl;
    int signalCount = 1;

    PythonHistogram raw_in {
            2,
            "Test signal name",
            "This is comment",
            "parrot",
            Time {
                    2021,
                    11,
                    3,
                    3,
                    15,
                    16,
                    12,
                    33
            },
            8
    };

    long dat[8] = {0, 1, 0, 2, 0, 1, 0, 1};

    double tMin = 0;
    double tMax = 7;
    double yMin = 0;
    double delta = 1;

    int flipSize = raw_in.nPoints;
    int total_size = sizeof(CombiscopeHistogram) - sizeof(unsigned char *);
    switch (raw_in.type>>16){
        case 0:
            total_size += raw_in.nPoints * sizeof(long);
            break;
        case 1:
            flipSize *= 4; // 2 from long->double; 2 from x and y
            total_size += raw_in.nPoints * sizeof(double) * 2;
            break;
        case 2:
            flipSize *= 6; // 2 from long->double; 3 from x, y and z
            total_size += raw_in.nPoints * sizeof(double) * 3;
            std::cout << "Not implemented. Please, give this .sht file to Nikita" << std::endl;
            break;
        default:
            std::cout << "WTF? Not implemented. Please, give this .sht file to Nikita" << std::endl;
            break;
    }

    auto* buffer = new unsigned char[total_size];
    auto* buffPosition = buffer;
    std::memcpy(buffPosition, &raw_in, sizeof(PythonHistogram));
    buffPosition += sizeof(PythonHistogram) - sizeof(unsigned char *);
    std::memcpy(buffPosition, &tMin, sizeof(double));
    buffPosition += sizeof(double);
    std::memcpy(buffPosition, &tMax, sizeof(double));
    buffPosition += sizeof(double);
    std::memcpy(buffPosition, &yMin, sizeof(double));
    buffPosition += sizeof(double);
    std::memcpy(buffPosition, &delta, sizeof(double));
    buffPosition += sizeof(double);

    auto* in = (CombiscopeHistogram *) buffer;
    auto lBuf = (long*)&dat;

    LongFlip flip{0};
    for(int i = 0; i < flipSize; i++){
        flip.asLong = lBuf[i];
        std::memcpy(buffPosition + i, &flip.asChar[0], sizeof(char));
        std::memcpy(buffPosition + i + in->nPoints, &flip.asChar[1], sizeof(char));
        std::memcpy(buffPosition + i + in->nPoints * 2, &flip.asChar[2], sizeof(char));
        std::memcpy(buffPosition + i + in->nPoints * 3, &flip.asChar[3], sizeof(char));
    }

    CompressedRLE* rle = compressRLE(in, total_size);
    delete[] buffer;

    CompressedHoff packed = compressHoffman(rle);
    delete rle;

    std::string inFilename = "d:/tmp/TS.SHT";
    std::ofstream outFile;
    outFile.open (inFilename, std::ios::out | std::ios::binary);
    if (outFile.is_open()){
        outFile.write(V2, sizeof(V2));
        outFile.write((const char*)&signalCount, sizeof(int));

        for(int i = 0; i < signalCount; i++){
            outFile.write((char *)&packed.size, sizeof(int));
            outFile.write(packed.data, packed.size);
            delete[] packed.data;
        }

        outFile.close();

        std::cout << "file written" << std::endl;
    }else{
        std::cout << "Unable to open file" << std::endl;
    }
    std::cout << "CPP pack OK" << std::endl;

    return;
}

int innerTest(const int n){
    return n * n;
}

void innerFreeOut(){
    delete out.point;
    out.size = 0;
    currentOutPos = nullptr;
}