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

void ChangeByteOrder(){
    char *Data1,*Data;
    int arr_begin,number_of_groups;
    int currentGroupIndex;

    int Level = 4;
    int DataSize = 1 * sizeof(double);
    double data[] = {123.321e2};
    double original[] = {123.321e2};

    Data=(char *)&data;
    Data1=(char *)GlobalAlloc(GMEM_FIXED, DataSize);


    arr_begin = 0;
    for (int insideGroupByte_index=0; insideGroupByte_index < 4; insideGroupByte_index++){
        number_of_groups = DataSize / 4;

        for (currentGroupIndex = 0; currentGroupIndex < number_of_groups; currentGroupIndex++){
            Data1[currentGroupIndex * 4 + insideGroupByte_index] = Data[arr_begin + currentGroupIndex];
        }

        arr_begin += number_of_groups;
    }

    for (currentGroupIndex=0; currentGroupIndex < DataSize; currentGroupIndex++){
        Data[currentGroupIndex]=Data1[currentGroupIndex];
    }
    GlobalFree(Data1);
}



int main(int argc, char* argv[]) {
    ChangeByteOrder();

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