#include "version.h"
#include <iostream>
#include "compress.h"

#include <fstream>

void delay(){
    for(int i  = 0; i < 1000000000; i++){
        double fuck = 1.0/i;
    }
}

void read(){
    std::cout << "CPP shtRipper, revision:" << REVISION << "\n" << std::endl << std::flush;

    std::string inFilename = "d:/data/cfm/original/sht40808.SHT";
    //std::string inFilename = "d:/tmp/TS.SHT";

    std::ifstream inFile;
    inFile.open (inFilename, std::ios::in | std::ios::binary | std::ios::ate);
    if (inFile.is_open()){
        std::streampos size = inFile.tellg();
        char * memblock = new char [size];
        inFile.seekg (0, std::ios::beg);
        inFile.read (memblock, size);
        inFile.close();

        std::cout << "the entire file content is in memory" << std::endl;

        //char sig[128] = "Лазер";
        char sig[128] = "Emission electrode voltage";
        auto readed = parseSHT(memblock, 1, sig);


        std::cout << "processed OK" << std::endl;

        delete[] memblock;
    }else{
        std::cout << "Unable to open file" << std::endl;
    }
}

void pack(){
    int signalCount = 2;
/*
    CombiscopeHistogram raw_in[2] = {{
            1<<16,
            "Test XY signal 1",
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
            2,
            1,
            2,
            2,
            5
    },
     {
             1<<16,
             "Test XY signal 2",
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
             2,
             1,
             2,
             2,
             5
     }};

    double dat[8] = {0, 1, 2, 3, 0, 3, 2, -3};
*/
    CombiscopeHistogram raw_in[2] = {{
                                             1<<16,
                                             "Test XY signal 1",
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
                                             2,
                                             1,
                                             2,
                                             2,
                                             5
                                     },
                                     {
                                             1<<16,
                                             "Test XY signal 2",
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
                                             2,
                                             1,
                                             2,
                                             2,
                                             5
                                     }};
    double dat[8] = {0, 1, 2, 3, 0, 3, 2, -3};

    std::string outFilename = "d:/tmp/TS.SHT";
    std::ofstream outFile;
    outFile.open (outFilename, std::ios::out | std::ios::binary);
    if (outFile.is_open()){
        Out out = packSHT(signalCount, (const char*)&raw_in, (const char*)&dat);
        outFile.write(out.point, out.size);
        outFile.close();
        innerFreeOut();
    }else{
        std::cout << "Unable to open file" << std::endl;
    }


    std::cout << "\nNormal pack." << std::endl << std::flush;
}

int main(int argc, char* argv[]) {
    read();
    //pack();

    std::cout << "\nNormal exit." << std::endl << std::flush;
    delay();
}