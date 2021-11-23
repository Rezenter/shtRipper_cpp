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

    //std::string inFile = "d:/data/cfm/original/00040032.dat";
    std::string inFilename = "d:/data/cfm/original/sht40808.SHT";

    std::ifstream inFile;
    inFile.open (inFilename, std::ios::in | std::ios::binary | std::ios::ate);
    if (inFile.is_open()){
        std::streampos size = inFile.tellg();
        char * memblock = new char [size];
        inFile.seekg (0, std::ios::beg);
        inFile.read (memblock, size);
        inFile.close();

        std::cout << "the entire file content is in memory" << std::endl;

        for(int i = 0; i < 1; i++){
            std::cout << i << ' ' << parseSHT(memblock).size << std::endl;
        }

        std::cout << "processed OK" << std::endl;

        delete[] memblock;
    }else{
        std::cout << "Unable to open file" << std::endl;
    }

    //std::cout << ImportNIIFAFile(inFile.c_str()) << std::endl;


    std::cout << "\nNormal exit." << std::endl << std::flush;
    delay();
}