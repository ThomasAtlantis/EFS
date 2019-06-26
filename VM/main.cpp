//
// Created by MAC on 2019/6/19.
//

#include "bios.h"

int main() {
    string exeName = BIOS().run();
    system("cls");
    WinExec(exeName.c_str(), SW_SHOW);
    return 0;
}