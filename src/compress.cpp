#include "compress.h"
#include <fstream>
#include <cstring>

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
    //std::cout << "DecompressRLE()" << std::endl;
    int j;
    int Delta;
    int NBytes = 0;
    int i = 0;
    //std::cout << "compressed size " << compressed->size << std::endl;
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
    //int tmp = NBytes;
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
    //std::cout << "DecompressRLE() OK" << std::endl;
    //auto wtf = (CombiscopeHistogram*)OutBuff;
    //std::cout << "npoints " << wtf->nPoints << ' ' << sizeof(wtf->nPoints) << std::endl;
    //std::cout << "NBytes " << tmp << ' ' << wtf->nPoints * sizeof(double) << std::endl;
    //std::cout << "size " << (tmp - sizeof(int)*2 - 128*3 - sizeof(Time) - sizeof(double)*4) << std::endl;
    //std::cout << "manual " << (tmp - sizeof(int)*2 - 128*3 - sizeof(Time) - sizeof(double)*4) / 4 << std::endl;

    // int arrSize = histogram->nPoints * sizeof(double);

    return (CombiscopeHistogram*)OutBuff;
}

bool CreateGraph(const CompressedHoff* signal, CompressionGraph *Graph){
    //std::cout << "CreateGraph()" << std::endl;
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
    //std::cout << "CreateGraph() OK" << std::endl;
    return true;
}

CompressedRLE* DecompressHoffman(const CompressedHoff* compressed){
    //std::cout << "DecompressHoffman()" << std::endl;
    //std::cout << "hoffman size " << compressed->size << std::endl;
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
    delete[] compressed->data;
    //std::cout << "will break old python-based read!" << std::endl;
    return RLE;
}

CombiscopeHistogram *DecompressHist(const CompressedHoff compressed){
    //std::cout << "DecompressHist()" << std::endl;
    CompressedRLE* signalRLE = DecompressHoffman(&compressed);
    if(signalRLE == nullptr){
        return nullptr;
    }
    //std::cout << "RLE size " << signalRLE->size << std::endl;
    auto *signal = DecompressRLE(signalRLE);
    delete signalRLE;

    //std::cout << "DecompressHist() OK" << std::endl;
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
        //std::cout << "worker gets data... " << tasks.size() << std::endl;
        CompressedHoff task = tasks.back();
        tasks.pop_back();
        //std::cout << "worker pop" << std::endl;

        lockIn.unlock();

        //std::cout << "worker decompressing... " << task.size << std::endl;
        signal = DecompressHist(task);
        //std::cout << "worker OK" << std::endl;
        if(signal != nullptr){
            appendOut(signal);
        }
        //std::cout << "worker next\n\n" << std::endl;
    }
}

void appendOut(const CombiscopeHistogram* histogram){
    //std::cout << "appendOut()" << std::endl;
    lockOut.lock();
    bool found = (reqCount == 0);
    for(int i = 0; i < reqCount; i++) {
        int char_i;
        for (char_i = 0; char_i < 128; char_i++) {
            if (histogram->name[char_i] != req[i * 128 + char_i]) {
                break;
            }
        }
        if(char_i == 128 || req[i * 128 + char_i] == 0)  {
            found = true;
            break;
        }
    }

    if(found) {
        outs[out.size].size = SIGNAL_HEADER_SIZE + (histogram->nPoints * 2) * sizeof(double);
        outs[out.size].point = new char[outs[out.size].size];

        std::memcpy(outs[out.size].point, histogram, SIGNAL_HEADER_SIZE);

        char* ptr = outs[out.size].point + SIGNAL_HEADER_SIZE;
        auto *dBuff = (double *) (ptr);
        out.size += 1;
        lockOut.unlock();

        switch (histogram->type >> 16) {
            case 0: {
                double tMult = (histogram->tMax - histogram->tMin) / (histogram->nPoints - 1);

                LongFlip flip{0};
                for (int i = 0; i < histogram->nPoints; i++) {
                    flip.asChar[0] = histogram->data[i];
                    flip.asChar[1] = histogram->data[histogram->nPoints + i];
                    flip.asChar[2] = histogram->data[histogram->nPoints * 2 + i];
                    flip.asChar[3] = histogram->data[histogram->nPoints * 3 + i];

                    dBuff[i] = i * tMult + histogram->tMin;
                    dBuff[i + histogram->nPoints] = flip.asLong * histogram->delta + histogram->yMin; // y coordinates
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
    }else{
        out.size += 1;
        lockOut.unlock();
    }
    //std::cout << "appendOut() OK" << std::endl;
    delete histogram;
}

Out parseSHT(const char* path, const unsigned int reqC, char* requests) { // adapted version of RestoreHist(..., int version)
    std::string p(path);
    //std::cout << "new OK: " << p << std::endl;

    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()){
        std::cout << "failed to open " << path << std::endl;
        out.size = -7;
        return out;
    }

    //std::cout << "c++ begin" << std::endl;
    innerFreeOut();
    int compressedSignalSize;

    reqCount = reqC;
    req = requests;

    char ver[12];
    input.read(ver, sizeof(V2));
    if (strcmp(ver, V2) != 0 or input.gcount() != sizeof(V2)){
        std::cout << "Bad file: " << ver << ' ' << input.gcount() << std::endl;
        out.size = -1;
        return out;
    }

    //size_t currentPos = sizeof(V1);

    int signalCount;
    input.read(reinterpret_cast<char*>(&signalCount), sizeof(signalCount));
    if (input.gcount() != sizeof(signalCount)){
        std::cout << "Bad file size: "  << ' ' << input.gcount() << std::endl;
        out.size = -1;
        return out;
    }
    //std::memcpy(&signalCount, in + currentPos, sizeof(int));
    //currentPos += sizeof(int);

    //std::cout << "signal count " << signalCount << std::endl;

    int totalInSize = 0;
    for(int signalIndex = 0; signalIndex < signalCount; signalIndex++){
        //std::cout << signalIndex << std::endl;

        //std::cout << "case 2" << std::endl;
        //std::cout << "read compressed signal " << signalIndex << std::endl;
        input.read(reinterpret_cast<char*>(&compressedSignalSize), sizeof(compressedSignalSize));
        if (input.gcount() != sizeof(compressedSignalSize)){
            std::cout << "Bad file size: "  << ' ' << input.gcount() << std::endl;
            break;
        }
        //std::cout << "compressed signal size: " << compressedSignalSize << std::endl;
        //std::memcpy(&compressedSignalSize, in + currentPos, sizeof(int));
        //std::cout << "memcpy OK" << std::endl;
        //std::cout << "fuck \n\n\n\n";
        //totalInSize += compressedSignalSize;
        //currentPos += sizeof(int);

        if(compressedSignalSize <= 0){
            //std::cout << "bad compressed signal size " << compressedSignalSize << std::endl;
            break;
        }
        //std::cout << "task " << signalIndex << " size " << compressedSignalSize << std::endl;
        char* compressed = new char[compressedSignalSize];
        input.read(compressed, compressedSignalSize);
        if (input.gcount() != compressedSignalSize){
            std::cout << "Bad file size: "  << ' ' << input.gcount() << ' ' << compressedSignalSize << std::endl;
            break;
        }
        tasks.push_back(CompressedHoff {
                                compressed,
                                compressedSignalSize
                        }
        );
        //std::cout << "Push ok" << std::endl;
        //currentPos += compressedSignalSize;
    }
    //std::cout << "tasks pushed"  << std::endl;
    input.close();

    if(!tasks.empty()){
        outs = new Out[tasks.size()];

        size_t threadCount = std::thread::hardware_concurrency();

        //std::cout << "starting workers: " << threadCount << std::endl;
        //threadCount = 1; //DEBUG!
        //std::cout << "\n\nWARNING!!! DEBUG SINGLE THREAD\n\n" << std::endl;


        out.size = 0;
        for(size_t i = 0; i < threadCount; i++){
            std::thread thread(worker);
            workers.push_back(move(thread));
        }

        //std::cout << "waiting..." << std::endl;

        for(std::thread &thread : workers){
            thread.join();
        }
        //std::cout << "joined" << std::endl;
        workers.clear();
    }

    int totalOutSize = 0;
    for(int signalIndex = 0; signalIndex < out.size; signalIndex++){
        totalOutSize += outs[signalIndex].size;
    }
    //std::cout << "totalOutSize " << totalOutSize << std::endl;
    out.point = new char[totalOutSize];
    currentOutPos = out.point;
    //std::cout << "allocated" << std::endl;
    for(int signalIndex = 0; signalIndex < out.size; signalIndex++){
        std::memcpy(currentOutPos, outs[signalIndex].point, outs[signalIndex].size);
        currentOutPos += outs[signalIndex].size;
        //std::cout << "del" << std::endl;
        delete[] outs[signalIndex].point;
    }
    //std::cout << "c++ OK " << out.size << std::endl;

    return out;
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
            (cSize + 511 + (int)sizeof(int))
    };
}

Out packSHT(const int signalCount, const char* headers, const char* data){
    innerFreeOut();

    auto* currData = (long*)data;
    auto outQueue = new Out[signalCount];
    int totalSize = sizeof(V2) + sizeof(int);
    for(int signalIndex = 0; signalIndex < signalCount; signalIndex++){
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
                break;
            default:
                std::cout << "WTF? Not implemented. Please, give this .sht file to Nikita" << std::endl;

                out.size = -2;
                return out;
        }

        int signalSize = sizeof(CombiscopeHistogram) - 8 + flipSize * sizeof(long);

        auto* buffer = new unsigned char[signalSize];
        std::memcpy(buffer, raw_in, sizeof(CombiscopeHistogram) - 8);
        auto* buffPosition = buffer + sizeof(CombiscopeHistogram) - 8;


        LongFlip flip{0};

        for (int i = 0; i < flipSize; i++) {
            flip.asLong = currData[i];
            std::memcpy(buffPosition + i, &flip.asChar[0], sizeof(char));
            std::memcpy(buffPosition + i + flipSize, &flip.asChar[1], sizeof(char));
            std::memcpy(buffPosition + i + flipSize * 2, &flip.asChar[2], sizeof(char));
            std::memcpy(buffPosition + i + flipSize * 3, &flip.asChar[3], sizeof(char));
        }
        currData += flipSize;

        CompressedRLE *rle = compressRLE((CombiscopeHistogram *) buffer, signalSize);
        delete[] buffer;

        CompressedHoff packed = compressHoffman(rle);
        delete rle;

        totalSize += packed.size + sizeof(int);
        outQueue[signalIndex] = Out{
                packed.size,
                const_cast<char *>(packed.data)
        };

    }

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

    return out;
}

Out pacADC(const int signalCount, const char* headers, const char* data){
    //COPYPASTED!

    innerFreeOut();

    auto* currData = (long*)data;
    auto outQueue = new Out[signalCount];
    int totalSize = sizeof(V2) + sizeof(int);
    for(int signalIndex = 0; signalIndex < signalCount; signalIndex++){
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
                break;
            default:
                std::cout << "WTF? Not implemented. Please, give this .sht file to Nikita" << std::endl;

                out.size = -2;
                return out;
        }

        int signalSize = sizeof(CombiscopeHistogram) - 8 + flipSize * sizeof(long);

        auto* buffer = new unsigned char[signalSize];
        std::memcpy(buffer, raw_in, sizeof(CombiscopeHistogram) - 8);
        auto* buffPosition = buffer + sizeof(CombiscopeHistogram) - 8;


        LongFlip flip{0};

        for (int i = 0; i < flipSize; i++) {
            flip.asLong = currData[i];
            std::memcpy(buffPosition + i, &flip.asChar[0], sizeof(char));
            std::memcpy(buffPosition + i + flipSize, &flip.asChar[1], sizeof(char));
            std::memcpy(buffPosition + i + flipSize * 2, &flip.asChar[2], sizeof(char));
            std::memcpy(buffPosition + i + flipSize * 3, &flip.asChar[3], sizeof(char));
        }
        currData += flipSize;

        CompressedRLE *rle = compressRLE((CombiscopeHistogram *) buffer, signalSize);
        delete[] buffer;

        CompressedHoff packed = compressHoffman(rle);
        delete rle;

        totalSize += packed.size + sizeof(int);
        outQueue[signalIndex] = Out{
                packed.size,
                const_cast<char *>(packed.data)
        };

    }

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

    return out;
}

int innerTest(const int n){
    return n * n;
}

void innerFreeOut(){
    if(out.size != 0){
        delete[] out.point;
        out.size = 0;
    }
    currentOutPos = nullptr;
    reqCount = 0;
    req = nullptr;
}