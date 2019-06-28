//
// Created by MAC on 2019/6/19.
//

#include "bios.h"

int main() {
    string exeName = BIOS().run();
    string command = exeName \
        .append(" -m ../SLOT_1/data.vhd") \
        .append(" -v ../SLOT_2/data.vhd") \
        .append(" -p ../SLOT_X/data.vhd");
    system("cls"); WinExec(exeName.c_str(), SW_SHOW);
    return 0;
}