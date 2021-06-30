#include "version.h"
#include <iostream>
#include "magn_file.h"

void delay(){
    for(int i  = 0; i < 100000000; i++){
        double fuck = 1.0/i;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "CPP shtRipper, revision:" << REVISION << "\n\n\n" << std::endl << std::flush;
    delay();
    std::cout << std::endl;

    std::string inFile = "d:/data/cfm/original/00040032.dat";
    std::cout << ImportNIIFAFile(inFile.c_str()) << std::endl;


    std::cout << "\nNormal exit." << std::endl << std::flush;
    delay();
}