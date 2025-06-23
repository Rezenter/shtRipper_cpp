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

void appendOut(const CombiscopeHistogram* histogram) {
    //std::cout << "appendOut()" << std::endl;
    lockOut.lock();
    bool found = (reqCount == 0);
    for (int i = 0; i < reqCount; i++) {
        int char_i;
        for (char_i = 0; char_i < 128; char_i++) {
            if (histogram->name[char_i] != req[i * 128 + char_i]) {
                break;
            }
        }
        if (char_i == 128 || req[i * 128 + char_i] == 0) {
            found = true;
            break;
        }
    }

    if (found) {
        switch (histogram->type >> 16) {
        case 0: {
            outs[out.size].size = SIGNAL_HEADER_SIZE + (histogram->nPoints * 2) * sizeof(double);
            break;
            }
        case 1: {
            outs[out.size].size = SIGNAL_HEADER_SIZE + (histogram->nPoints * 2) * sizeof(double);
            break;
            }
        case 2: {
            outs[out.size].size = SIGNAL_HEADER_SIZE + (histogram->nPoints * 3) * sizeof(double);
            break;
            }
        default: {
            std::cout << "Unknown type" << std::endl;
            outs[out.size].size = SIGNAL_HEADER_SIZE;
            break;
            }
        }
        //outs[out.size].size = SIGNAL_HEADER_SIZE + (histogram->nPoints * 2) * sizeof(double);
        //std::cout << "appendOut() out size: " << outs[out.size].size << std::endl;
        outs[out.size].point = new char[outs[out.size].size];

        std::memcpy(outs[out.size].point, histogram, SIGNAL_HEADER_SIZE);

        char* ptr = outs[out.size].point + SIGNAL_HEADER_SIZE;
        auto *dBuff = (double *) (ptr);
        auto* lBuff = (long*)(ptr);
        out.size += 1;
        lockOut.unlock();
        LongFlip flip{ 0 };
        LongToDouble convert{ 0 };
        switch (histogram->type >> 16) {
            case 0: {
                double tMult = (histogram->tMax - histogram->tMin) / (histogram->nPoints - 1);
                for (int i = 0; i < histogram->nPoints; i++) {
                    flip.asChar[0] = histogram->data[i];
                    flip.asChar[1] = histogram->data[histogram->nPoints + i];
                    flip.asChar[2] = histogram->data[histogram->nPoints * 2 + i];
                    flip.asChar[3] = histogram->data[histogram->nPoints * 3 + i];

                    dBuff[i] = i * tMult + histogram->tMin;
                    dBuff[i + histogram->nPoints] = (double)flip.asLong * histogram->delta + histogram->yMin; // y coordinates
                }
                break;
            }
            case 1:
                for (int i = 0; i < histogram->nPoints; i++) {
                    flip.asChar[0] = histogram->data[ i * 4];
                    flip.asChar[1] = histogram->data[(i * 4 + histogram->nPoints * 4)];
                    flip.asChar[2] = histogram->data[(i * 4 + histogram->nPoints * 4 * 2)];
                    flip.asChar[3] = histogram->data[(i * 4 + histogram->nPoints * 4 * 3)];
                    convert.asLong[0] = flip.asLong;
                    flip.asChar[0] = histogram->data[ i * 4 + 1];
                    flip.asChar[1] = histogram->data[(i * 4 + 1 + histogram->nPoints * 4)];
                    flip.asChar[2] = histogram->data[(i * 4 + 1 + histogram->nPoints * 4 * 2)];
                    flip.asChar[3] = histogram->data[(i * 4 + 1 + histogram->nPoints * 4 * 3 )];
                    convert.asLong[1] = flip.asLong;
                    dBuff[i] = convert.asDouble;
                    
                    flip.asChar[0] = histogram->data[ i * 4 + 2];
                    flip.asChar[1] = histogram->data[(i * 4 + 2 + histogram->nPoints * 4)];
                    flip.asChar[2] = histogram->data[(i * 4 + 2 + histogram->nPoints * 4 * 2)];
                    flip.asChar[3] = histogram->data[(i * 4 + 2 + histogram->nPoints * 4 * 3)];
                    convert.asLong[0] = flip.asLong;
                    flip.asChar[0] = histogram->data[ i * 4 + 3];
                    flip.asChar[1] = histogram->data[(i * 4 + 3 + histogram->nPoints * 4)];
                    flip.asChar[2] = histogram->data[(i * 4 + 3 + histogram->nPoints * 4 * 2)];
                    flip.asChar[3] = histogram->data[(i * 4 + 3 + histogram->nPoints * 4 * 3)];
                    convert.asLong[1] = flip.asLong;
                    dBuff[i + histogram->nPoints] = convert.asDouble;
                }
                break;
            case 2:
                for (int i = 0; i < histogram->nPoints; i++) {
                    flip.asChar[0] = histogram->data[i * 6];
                    flip.asChar[1] = histogram->data[(i * 6 + histogram->nPoints * 6)];
                    flip.asChar[2] = histogram->data[(i * 6 + histogram->nPoints * 6 * 2)];
                    flip.asChar[3] = histogram->data[(i * 6 + histogram->nPoints * 6 * 3)];
                    convert.asLong[0] = flip.asLong;
                    flip.asChar[0] = histogram->data[i * 6 + 1];
                    flip.asChar[1] = histogram->data[(i * 6 + 1 + histogram->nPoints * 6)];
                    flip.asChar[2] = histogram->data[(i * 6 + 1 + histogram->nPoints * 6 * 2)];
                    flip.asChar[3] = histogram->data[(i * 6 + 1 + histogram->nPoints * 6 * 3)];
                    convert.asLong[1] = flip.asLong;
                    dBuff[i] = convert.asDouble;

                    flip.asChar[0] = histogram->data[i * 6 + 2];
                    flip.asChar[1] = histogram->data[(i * 6 + 2 + histogram->nPoints * 6)];
                    flip.asChar[2] = histogram->data[(i * 6 + 2 + histogram->nPoints * 6 * 2)];
                    flip.asChar[3] = histogram->data[(i * 6 + 2 + histogram->nPoints * 6 * 3)];
                    convert.asLong[0] = flip.asLong;
                    flip.asChar[0] = histogram->data[i * 6 + 3];
                    flip.asChar[1] = histogram->data[(i * 6 + 3 + histogram->nPoints * 6)];
                    flip.asChar[2] = histogram->data[(i * 6 + 3 + histogram->nPoints * 6 * 2)];
                    flip.asChar[3] = histogram->data[(i * 6 + 3 + histogram->nPoints * 6 * 3)];
                    convert.asLong[1] = flip.asLong;
                    dBuff[i + histogram->nPoints] = convert.asDouble;

                    flip.asChar[0] = histogram->data[i * 6 + 4];
                    flip.asChar[1] = histogram->data[(i * 6 + 4 + histogram->nPoints * 6)];
                    flip.asChar[2] = histogram->data[(i * 6 + 4 + histogram->nPoints * 6 * 2)];
                    flip.asChar[3] = histogram->data[(i * 6 + 4 + histogram->nPoints * 6 * 3)];
                    convert.asLong[0] = flip.asLong;
                    flip.asChar[0] = histogram->data[i * 6 + 5];
                    flip.asChar[1] = histogram->data[(i * 6 + 5 + histogram->nPoints * 6)];
                    flip.asChar[2] = histogram->data[(i * 6 + 5 + histogram->nPoints * 6 * 2)];
                    flip.asChar[3] = histogram->data[(i * 6 + 5 + histogram->nPoints * 6 * 3)];
                    convert.asLong[1] = flip.asLong;
                    dBuff[i + histogram->nPoints * 2] = convert.asDouble;
                }
                break;
            default:
                break;
        }
    }else{
        //out.size += 1;
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
        out.size = 0;
        return out;
    }

    //std::cout << "c++ begin" << std::endl;
    innerFreeOut();
    size_t compressedSignalSize;

    reqCount = reqC;
    req = requests;

    char ver[12];
    input.read(ver, sizeof(V2));
    if (strcmp(ver, V2) != 0 or input.gcount() != sizeof(V2)){
        std::cout << "Bad file: " << ver << ' ' << input.gcount() << std::endl;
        out.size = 0;
        return out;
    }

    //size_t currentPos = sizeof(V1);

    int signalCount;
    input.read(reinterpret_cast<char*>(&signalCount), sizeof(signalCount));
    if (input.gcount() != sizeof(signalCount)){
        std::cout << "Bad file size: "  << ' ' << input.gcount() << std::endl;
        out.size = 0;
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
        uint32_t tmp;
        input.read(reinterpret_cast<char*>(&tmp), sizeof(tmp));
        if (input.gcount() != sizeof(tmp)){
            std::cout << "Bad file size: "  << ' ' << input.gcount() << std::endl;
            break;
        }
        compressedSignalSize = (size_t)tmp;
        //std::cout << "compressed signal size: " << compressedSignalSize << std::endl;
        //std::memcpy(&compressedSignalSize, in + currentPos, sizeof(int));
        //std::cout << "memcpy OK" << std::endl;
        //std::cout << "fuck \n\n\n\n";
        //totalInSize += compressedSignalSize;
        //currentPos += sizeof(int);

        if(compressedSignalSize <= 0){
            std::cout << "bad compressed signal size " << compressedSignalSize << std::endl;
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

    size_t totalOutSize = 0;
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
            (size_t)(cSize + 511 + (size_t)sizeof(int))
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
                break;          //type for raw ADC registers. X scale is defined by t_min, t_max and point_count. Y is defined by y_min, y_max and data array
                                //this is the original combiscope format. Other types do not support most of the operations.
            case 1:
                flipSize *= 4; // 2 from long->double; 2 from x and y
                                //both x and y are defined by arrays. Allows irregular X steps
                break;
            case 2:
                flipSize *= 6; // 2 from long->double; 3 from x, y and z
                                // allows to store Y error bars
                break;
            default:
                std::cout << "WTF? Not implemented. Please, give this *.sht file to Nikita" << std::endl;
                out.size = 0;
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

Out packADC(const int signalCount, const char* headers, const char* data) {
    const uint8_t chMap[16] = {0, 2, 4, 6, 10, 8, 14, 12, 1, 3, 5, 7, 11, 9, 15, 13};
  //const uint8_t chMap[16] = {0, 8, 1, 9, 2, 10, 3, 11, 5, 13, 4, 12, 7, 15, 6, 14};

    innerFreeOut();
    uint32_t pointCount = 0;
    char* compressed;
    bool deleteMe = false;
    if(signalCount == 0){
        //std::cout << data << std::endl;
        //data = path;

        std::string p(data);
        //std::cout << "pack ADC from file: " << p << std::endl;

        std::ifstream input(p, std::ios::binary);
        if (!input.is_open()) {
            std::cout << "failed to open " << p << std::endl;
            out.size = 0;
            return out;
        }
        input.seekg(0, std::ios_base::end);
        uint32_t rawSize = input.tellg();
        input.seekg(0, std::ios_base::beg);
        if (rawSize % (16 * 2) != 0) {
            std::cout << "File size is not X*16*2 bytes: " << rawSize % (16 * 2) << ' ' << rawSize << std::endl;
            out.size = 0;
            return out;
        }
        pointCount = rawSize / (16 * 2);
        deleteMe = true;
        compressed = new char[rawSize];
        input.read(compressed, rawSize);
        input.close();
        if (input.gcount() != rawSize) {
            std::cout << "Bad file size: " << input.gcount() << ' ' << rawSize << std::endl;
            delete[] compressed;
            out.size = 0;
            return out;
        }
    }else{ 
        //data = data;

        std::cout << "pack ADC: data is data "  << std::endl;
        /*
        if (rawSize % (16 * 2) != 0) {
            std::cout << "File size is not X*16*2 bytes: " << rawSize % (16 * 2) << ' ' << rawSize << std::endl;
            out.size = 0;
            return out;
        }*/
    }


    auto* asRegister = (int16_t*)(compressed);
    auto outQueue = new Out[16];
    int totalSize = sizeof(V2) + sizeof(int);
    const char zero = 0;
    LongFlip flip;
    int sc = 0;
    for (int signalIndex = 0; signalIndex < 16; signalIndex++) {
        auto* raw_in = (CombiscopeHistogram*)(headers + sizeof(CombiscopeHistogram) * signalIndex);
        if (raw_in->tMax < raw_in->tMin) {
            //std::cout << "skip ch = " << signalIndex << std::endl;
            continue;
        }
        
        raw_in->nPoints = pointCount;
        raw_in->tMax = raw_in->tMin + pointCount / (5e5 * 0.987652); // 500kHz with correction

        int signalSize = sizeof(CombiscopeHistogram) - 8 + pointCount * sizeof(long);
        auto* buffer = new unsigned char[signalSize];
        std::memcpy(buffer, raw_in, sizeof(CombiscopeHistogram) - 8);
        auto* buffPosition = buffer + sizeof(CombiscopeHistogram) - 8;

        for (uint32_t i = 0; i < pointCount; i++) {

            flip.asLong = asRegister[16 * i + chMap[signalIndex]];
            std::memcpy(buffPosition + i, &flip.asChar[0], sizeof(char));
            std::memcpy(buffPosition + i + pointCount, &flip.asChar[1], sizeof(char));
            std::memcpy(buffPosition + i + pointCount * 2, &flip.asChar[2], sizeof(char));
            std::memcpy(buffPosition + i + pointCount * 3, &flip.asChar[3], sizeof(char));
        }

        CompressedRLE* rle = compressRLE((CombiscopeHistogram*)buffer, signalSize);
        delete[] buffer;

        CompressedHoff packed = compressHoffman(rle);
        delete rle;

        totalSize += packed.size + sizeof(int);
        outQueue[signalIndex] = Out{
                packed.size,
                const_cast<char*>(packed.data)
        };
        sc++;
    }

    if (deleteMe) {
        delete[] compressed;
        deleteMe = false;
    }

    out.point = new char[totalSize];
    std::memcpy(out.point, V2, sizeof(V2));
    out.size += sizeof(V2);

    std::memcpy(out.point + out.size, &sc, sizeof(int));
    out.size += sizeof(int);
    for (int signalIndex = 0; signalIndex < 16; signalIndex++) {
        auto* raw_in = (CombiscopeHistogram*)(headers + sizeof(CombiscopeHistogram) * signalIndex);
        if (raw_in->tMax < raw_in->tMin) {
            //std::cout << "skip ch = " << signalIndex << std::endl;
            continue;
        }

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