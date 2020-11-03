#include <boost/process.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <windows.h>
#include <stdio.h>
namespace bp = boost::process;

std::mutex mz, mr;
std::condition_variable cvz;

const int TOTALTHREADS = 2;
int finished = 0;
bool gotZero = false;
bool terminated = false;
int computations[2];
bp::ipstream in_pipes[2];

void printEscExitMessage(bp::child* processes) {   
    std::string exitMessage = "Esc was pressed\n";

    terminated = true;
    
    if (processes[0].running()) {
        exitMessage += "Function f(x) hasn't been computed\n";
        processes[0].terminate();
    }
    if (processes[1].running()) {
        exitMessage += "Function g(x) hasn't been computed\n";
        processes[1].terminate();
    }

    std::cout << exitMessage;
    exit(0);
}

void printResultExitMessage() {
    std::string exitMessage = "The result is ";
    bool result = gotZero ? false : computations[0] && computations[1];
    exitMessage += std::to_string(result);

    std::cout << exitMessage << std::endl;
    exit(0);
}

bool procKeyEvent(KEY_EVENT_RECORD ker) {
    //Esc button was pressed
    if (ker.uChar.AsciiChar == 27) {
        return true;
    }

    return false;
}

void escHandler(bp::child* processes) {
    HANDLE hStdin;
    DWORD cNumRead, i;
    INPUT_RECORD irInBuf[128];
    int counter = 0;

    // Get the standard input handle. 
    hStdin = GetStdHandle(STD_INPUT_HANDLE);

    // Loop to read and handle the next 100 input events. 
    while (1) {   
        // Wait for the events. 

        ReadConsoleInput(
            hStdin,      // input buffer handle 
            irInBuf,     // buffer to read into 
            128,         // size of read buffer 
            &cNumRead); // number of records read 

        // Dispatch the events to the appropriate handler. 
        for (i = 0; i < cNumRead; i++) {       
            if (irInBuf[i].EventType == KEY_EVENT) {
                if (procKeyEvent(irInBuf[i].Event.KeyEvent)) {
                    printEscExitMessage(processes);
                }
            }           
        }
    }
}


void processHandler(const int i) {
    int computation;
    in_pipes[i] >> computation;

    //check if processes were terminated
    if (terminated)
        return;

    //if we got zero
    if (computation == 0) {
        gotZero = true;
        cvz.notify_one();
        return;
    }

    //remember result and check if thread was last to finish
    bool last = false;
    {
        std::lock_guard<std::mutex> lk(mr);
        computations[i] = computation;
        ++finished;
        if (finished == TOTALTHREADS)
            last = true;
    }

    if (last)
        printResultExitMessage();
}


int main(int argc, char* argv[]) {
    bp::opstream out_pipes[2];
    bp::child processes[2];      

    std::cout << "Enter x... ";
    int x;
    std::cin >> x;
     
    //starting processes
    for (int i = 0; i < 2; ++i) {
        processes[i] = bp::child("Function.exe", bp::std_out > in_pipes[i], bp::std_in < out_pipes[i]);
    }
    
    //starting to monitor esc key
    std::thread escListener(escHandler, processes);
    escListener.detach();

    //passing parameters to processes
    out_pipes[0] << 'f' << std::endl << x <<std::endl;
    out_pipes[0].close();
    out_pipes[1] << 'g' << std::endl << x << std::endl;   
    out_pipes[1].close();

    //starting threads to monitor processes results
    std::thread thr[2];
    for (int i = 0; i < 2; ++i) {
        thr[i] = std::thread(processHandler, i);
    }
    
    //check if we got zero
    {
        std::unique_lock<std::mutex> lk(mz);
        cvz.wait(lk, [] {return gotZero; });
    }
    terminated = true;
    for (int i = 0; i < 2; ++i) {
        processes[i].terminate();
    }
    printResultExitMessage();
}