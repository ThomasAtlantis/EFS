//
// Created by 12203 on 2019/6/28.
//

#ifndef EFS_SCREEN_H
#define EFS_SCREEN_H

#include <afxres.h>
#include "../../utilities.h"

class Screen {
public:
    Screen(){}
    // Demo:
    // _screen.cprintf((char*)str.data(), FOREGROUND_RED | FOREGROUND_GREEN);
    void cprintf(const char* str, WORD color, int wide = 0) {
        WORD colorOld;
        HANDLE handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(handle, &csbi);
        colorOld = csbi.wAttributes;
        SetConsoleTextAttribute(handle, color);
        if (wide == 0) std::cout << str;
        else std::cout << std::setw(wide) << str;
        SetConsoleTextAttribute(handle, colorOld);
    }
};



#endif //EFS_SCREEN_H
