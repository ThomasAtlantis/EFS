//
// Created by MAC on 2019/6/19.
//

#ifndef EFS_BOOTLOADER_H
#define EFS_BOOTLOADER_H

#include <cstdlib>
#include <windows.h>
#include <shellapi.h>
#include "..\utilities.h"

class BootLoader {
private:
    bool trouble;
    string OSLocation;
    string majorDisk;
    void troubleTip(string error, string success) {
        if (trouble) {
            cout << error << endl;
            cout << "press any key to shutdown machine ... " << endl;
            getchar();
            exit(1);
        } else {
            cout << success << endl;
            trouble = true;
        }
    }
    void checkTip(string check) {
        cout << check << " ... ";
        Sleep(800);
    }
    #define _check(c, e, s, sth) do{ \
        checkTip(c); \
        do{sth}while(0); \
        troubleTip(e,s); \
    } while(0)
public:
    BootLoader(): trouble(true) {
        vector<string> files = FileTool().dir("./SLOT_1");
        _check("checking major hard disk", "not detected!", "done!", {
            for (const auto &fileName: files) {
                if (FileTool().ext(fileName) == "img") {
                    majorDisk = fileName;
                    trouble = false;
                    break;
                }
            }
        });
        _check("checking operating system", "not detected!", "done!", {
            for (const auto &fileName: files) {
                if (FileTool().ext(fileName) == "exe") {
                    OSLocation = fileName;
                    trouble = false;
                    break;
                }
            }
        });
        _check("loading operating system", "failed!", "done!", {
            HINSTANCE hNewExe = ShellExecute(NULL, "open", OSLocation.c_str(), NULL, NULL, SW_SHOW);
            trouble = false;
        });
    }
};


#endif //EFS_BOOTLOADER_H
