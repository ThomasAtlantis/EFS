//
// Created by 12203 on 2019/6/28.
//

#ifndef EFS_VIM_H
#define EFS_VIM_H

#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <iostream>
using namespace std;
HANDLE hout;
//获得输入
char getInput()
{
    int ch; //输入字符串
    COORD coord; //屏幕上的坐标
    CONSOLE_SCREEN_BUFFER_INFO csbi; //控制台屏幕缓冲区信息
    coord.X=10;
    coord.Y=10;
    ch=getch();
    //0x0d表示回车，0XE0表示上下左右等键的键码
    while(ch==0xE0||ch==0x0d)
    {
        GetConsoleScreenBufferInfo(hout,&csbi);
        coord.X=csbi.dwCursorPosition.X; //得到坐标X的值
        coord.Y=csbi.dwCursorPosition.Y; //得到坐标Y的值
        //如果是回车
        if(ch==0x0d)
        {
            coord.X=0;
            coord.Y++;
            SetConsoleCursorPosition(hout,coord);
            break;
        }
        ch=getch();
        //上
        if(ch==0x48)
        {
            if(coord.Y!=0)coord.Y--;
        }
            //下
        else if(ch==0x50)
        {
            coord.Y++;
        }
            //左
        else if(ch==0x4b)
        {
            if(coord.X!=0){coord.X--;}
        }
            //右
        else if(ch==0x4d)
        {
            if(coord.X!=79)coord.X++;
            else
            {
                coord.X=0;
                coord.Y++;
            }
        }
        SetConsoleCursorPosition(hout,coord);
        ch=getch();
    }
    return ch;
}
void vim(string& str)
{
    cout<<str;
    char ch;
    hout=GetStdHandle(STD_OUTPUT_HANDLE);
    //从键盘获取输入，如果是方向键则执行方向功能，如果是回车键则换行，如果是字符则输出
    while(1)
    {
        ch=getInput();
        if(ch == 'Q'){
            return;
        }
        printf("%c",ch);
    }
}

#endif //EFS_VIM_H
