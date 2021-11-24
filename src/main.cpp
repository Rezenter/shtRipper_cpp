#include "version.h"
#include <iostream>
#include "magn_file.h"
#include "compress.h"

#include <fstream>

void delay(){
    for(int i  = 0; i < 100000000; i++){
        double fuck = 1.0/i;
    }
}


int main(int argc, char* argv[]) {
    std::cout << "CPP shtRipper, revision:" << REVISION << "\n" << std::endl << std::flush;

    /*
    HINSTANCE hinstLib = LoadLibrary(TEXT("cygripperForPython.dll"));
    std::cout << "dll loaded OK" << std::endl;
*/

    delay();
    std::cout << std::endl;


    int signalCount = 1;
    CombiscopeHistogram raw_in {
            1<<16,
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
            8,
            0,
            7,
            0,
            2
    };

    double dat[16] = {0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 0, 1, 2, 3, 4, 0, 1, 2};

    packSHT(signalCount, (const char*)&raw_in, (const char*)&dat);

    std::cout << "\nNormal pack." << std::endl << std::flush;
    delay();

    return 0;

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


        auto readed = parseSHT(memblock);


        std::cout << "processed OK" << std::endl;

        delete[] memblock;
    }else{
        std::cout << "Unable to open file" << std::endl;
    }

    //std::cout << ImportNIIFAFile(inFile.c_str()) << std::endl;


    std::cout << "\nNormal exit." << std::endl << std::flush;
    delay();
}