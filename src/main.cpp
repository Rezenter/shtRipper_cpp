void delay(){
    for(int i  = 0; i < 100000000; i++){
        double fuck = 1.0/i;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "TS fast acquisition, revision:" << REVISION << "\n\n\n" << std::endl << std::flush;
    delay();
    std::cout << std::endl;



    std::cout << "\nNormal exit." << std::endl << std::flush;
    delay();
}