#include "demofuncs"
#include "boost/process.hpp"
#include <iostream>
#include <string>
namespace bp = boost::process;

int main(int argc, char* argv[]) {
    char mode = 'f';
    std::cin >> mode;
    
    int x;
    std::cin >> x;
    
    int y = 0;

    if (mode == 'f') {
        y = spos::lab1::demo::f_func<spos::lab1::demo::AND>(x);
    }
    else if (mode == 'g') {
        y = spos::lab1::demo::g_func<spos::lab1::demo::AND>(x);        
    }
    
    std::cout << y;
    
    bp::std_out.close();

    exit (0);
}