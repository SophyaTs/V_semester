#include <boost/process.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <windows.h>
#include <stdio.h>

using std::cout, std::endl, std::string;
namespace bp = boost::process;

HANDLE hStdin;
DWORD fdwSaveOldMode;

bp::child processes[2];

//std::mutex exit_lock;
bool exit_pressed = false;

VOID KeyEventProc(KEY_EVENT_RECORD ker) {
    //ESC button was pressed
    if (ker.uChar.AsciiChar == 27) {
        std::string exitMessage = "Exit was pressed\n";
        if (processes[0].running()) {
            exitMessage += "Function f hasn't been computed\n";
            processes[0].terminate();
        }
        if (processes[1].running()) {
            exitMessage += "Function g hasn't been computed\n";
            processes[1].terminate();
        }
        
        cout << exitMessage;
        exit_pressed = true;
    }
}

void inputHandler() {
    DWORD cNumRead, fdwMode, i;
    INPUT_RECORD irInBuf[128];
    int counter = 0;

    // Get the standard input handle. 
    hStdin = GetStdHandle(STD_INPUT_HANDLE);

    // Save the current input mode, to be restored on exit. 
    GetConsoleMode(hStdin, &fdwSaveOldMode);

    // Enable the window and mouse input events. 
    fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
    SetConsoleMode(hStdin, fdwMode);

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
                KeyEventProc(irInBuf[i].Event.KeyEvent);
                if (exit_pressed) {
                    // Restore input mode on exit.
                    SetConsoleMode(hStdin, fdwSaveOldMode);
                    exit(0);
                }
            }           
        }
    }
}


int main(int argc, char* argv[]) {
    bp::ipstream in_pipes[2];
    int computations[2];
    bool result = true;

    int x;
    std::cin >> x;

    std::string launch_parameters = "Function.exe f " + std::to_string(x);
    processes[0] = bp::child(launch_parameters, bp::std_out > in_pipes[0]);
    
    launch_parameters = "Function.exe g " + std::to_string(x);
    processes[1] = bp::child(launch_parameters, bp::std_out > in_pipes[1]);
    processes[0].detach();
    processes[1].detach();

    std::thread exitListener(inputHandler);
    exitListener.detach();

    while (processes[0].running() && processes[1].running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
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

    if (processes[other].running()) {
        while (processes[other].running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        in_pipes[other] >> computations[other];
        //cout << "Got the other result: " << computations[other] << endl;
    }
    
    if (result != false)
        result = computations[0] && computations[1];
    
    cout << "The final result: " << result << endl;
    
    return 0;
}