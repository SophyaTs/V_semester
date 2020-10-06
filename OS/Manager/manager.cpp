#include <boost/process.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <windows.h>
#include <stdio.h>
namespace bp = boost::process;


bool procKeyEvent(KEY_EVENT_RECORD ker) {
    //Esc button was pressed
    if (ker.uChar.AsciiChar == 27) {
        return true;
    }

    return false;
}

std::string composeExitMessage(bp::child* processes) {
    std::string exitMessage = "Esc was pressed\n";
    if (processes[0].running()) {
        exitMessage += "Function f(x) hasn't been computed\n";
        processes[0].terminate();
    }
    if (processes[1].running()) {
        exitMessage += "Function g(x) hasn't been computed\n";
        processes[1].terminate();
    }

    return exitMessage;
}

void inputHandler(bp::child* processes) {
    HANDLE hStdin;
    DWORD cNumRead, fdwMode, i;
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
                    std::cout << composeExitMessage(processes) << std::endl;
                    exit(0);
                }
            }           
        }
    }
}


int main(int argc, char* argv[]) {
    bp::ipstream in_pipes[2];
    bp::child processes[2];
    int computations[2];
    bool result = true;

    std::cout << "Enter x... ";
    int x;
    std::cin >> x;
     
    //starting processes
    std::string launch_parameters = "Function.exe f " + std::to_string(x);
    processes[0] = bp::child(launch_parameters, bp::std_out > in_pipes[0]);   
    launch_parameters = "Function.exe g " + std::to_string(x);
    processes[1] = bp::child(launch_parameters, bp::std_out > in_pipes[1]);    

    std::thread escListener(inputHandler, processes);
    escListener.detach();

    //waiting for at least one of processes to finish
    while (processes[0].running() && processes[1].running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    //check if we got zero
    int other = 0;
    for(int i=0;i<2;++i)
    if (!processes[i].running()) {
        other = 1-i;
        in_pipes[i] >> computations[i];
        if (computations[i] == 0) {
            processes[other].terminate();
            result = false;
        }       
    }

    //waiting for other process to finish if needed
    if (processes[other].running()) {
        processes[other].wait();
        in_pipes[other] >> computations[other];
    }
    
    //just making sure that both processes had exited 
    processes[0].wait();
    processes[1].wait();

    if (result != false)
        result = computations[0] && computations[1];
    
    std::cout << "The final result: " << result << std::endl;
    
    return 0;
}