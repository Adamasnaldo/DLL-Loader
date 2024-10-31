// DummyEXE.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <thread>

int main( ) {

    int counter = 10;
    do {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::cout << "Dummy EXE is running for " << --counter << " seconds!" << "\r";
    } while (counter > 0);
    
    return 0;
}