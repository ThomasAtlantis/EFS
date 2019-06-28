//
// Created by 12203 on 2019/6/28.
//

#ifndef EFS_LIGHT_H
#define EFS_LIGHT_H

#include <afxres.h>
#include "../../utilities.h"

class Light
{
public:
    Light(){}
    void cprintf(char* str, WORD color, ...) {
        WORD colorOld;
        HANDLE handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(handle, &csbi);
        colorOld = csbi.wAttributes;
        SetConsoleTextAttribute(handle, color);
        std::cout << str;
        SetConsoleTextAttribute(handle, colorOld);
    }
};



#endif //EFS_LIGHT_H
