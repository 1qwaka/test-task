#include <iostream>
#include <fstream>
#include <array>

#include "vfs.h"

using std::cout;
using std::cerr;
using std::boolalpha;
using std::endl;

int main() {

    TestTask::VFS vfs;

    vfs.test();

    // std::fstream file;
    // file.open("coolfile", std::ios::out | std::ios::binary);

    // cout << boolalpha << "good: " << file.good() << "  is open: " << file.is_open() << endl;
    // char str[] = "123422";
    // auto prev_pos = file.tellp();
    // file.write(str, std::size(str));
    // cout << boolalpha << "wrote: " << (file.tellp() - prev_pos) << "  good: " << file.good() << endl;
    


    // std::cout << "hello" << std::endl;

    return 0;
}