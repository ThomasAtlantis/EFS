//
// Created by MAC on 2019/6/19.
//

#ifndef EFS_BIOS_H
#define EFS_BIOS_H

#include <cstdlib>
#include <windows.h>
#include <shellapi.h>
#include "tiny_efs.h"

class BIOS {
private:
    bool trouble;
    string OSLocation;
    string majorDisk;
    string majorSlot;
    string USBSlot;
    FSController * _fsc, * _fsc_usb;
    VHDController * _vhdc, * _vhdc_usb;
    char _buff[BIOS_BUF_BLOCK_SIZE * BLOCK_SIZE];
public:
    BIOS(): trouble(true), majorSlot("./SLOT_1/"), USBSlot("./SLOT_X/") {}
    void test() {
        _vhdc = new VHDController("./SLOT_2/");
        _fsc = new FSController(*_vhdc, OS_PART_BLOCK_SIZE, true);
        INode iNode;
        unsigned int length = 10;
        strcpy(_buff, string(R"(hello world!)").append("\0").c_str());
        _fsc->createFile(iNode, "hello.txt");
        _fsc->writeFileFromBuf(iNode, _buff, length);
        _fsc->flushDisk();
        _fsc->readFileToBuf(iNode.bid, _buff);
        cout << _buff << endl;
    }
    void run() {
        vector<string> files = FileTool().dir(majorSlot);
        cout << "checking major hard disk ... ";
        for (const auto &fileName: files) {
            if (FileTool().ext(fileName) == "vhd") {
                majorDisk = majorSlot + fileName;
                trouble = false;
                break;
            }
        }
        if (trouble) {
            cout << "not detected!" << endl;
            cout << "press any key to shutdown the machine >> ";
            getchar(); exit(0);
        } else {
            cout << "done!" << endl;
            trouble = true;
        }
        _vhdc = new VHDController(majorDisk);
        _fsc = new FSController(*_vhdc, OS_PART_BLOCK_SIZE);
        while (true) {
            cout << "install operating system from USB? Y/N" << endl;
            char op; cin >> op;
            if (static_cast<char>(toupper(op)) == 'Y') {
                vector<string> USBs, USBDisks;
                cout << "scanning USB devices ... ";
                for (auto &USB: FileTool().dir(USBSlot)) {
                    vector<string> USBfiles = FileTool().dir(USBSlot + USB);
                    for (const auto &fileName: USBfiles) {
                        if (FileTool().ext(fileName) == "vhd") {
                            USBs.push_back(USB.append("/").append(fileName));
                            USBDisks.push_back(USBSlot + USB);
                            trouble = false;
                            break;
                        }
                    }
                }
                if (trouble) {
                    cout << "not detected!" << endl;
                    continue;
                }
                cout << "done!" << endl; trouble = true;
                cout << "select a USB for installation: " << endl;
                for (size_t i = 0; i < USBs.size(); ++ i)
                    cout << "[" << i << "] " << USBs[i] << endl;
                size_t option; cin >> option;
                if (option >= 0 && option < USBs.size()) {
                    _vhdc_usb = new VHDController(USBDisks[option]);
                    _fsc_usb = new FSController(*_vhdc_usb, OS_PART_BLOCK_SIZE);
//                    INode tmp;
//                    _fsc_usb->createFile(tmp, "ios.iso");
//                    _fsc_usb->flushDisk();
                    cout << "scanning operating system mirrors ... ";
                    vector<INode> isoList = _fsc_usb->dir();
                    for (const auto &file: isoList) {
                        if (FileTool().ext(file.name) == "iso") {
                            cout << "installing ..." << endl;
                            INode iNode;
                            _fsc_usb->readFileToBuf(file.bid, _buff);
                            _fsc->createFile(iNode, FileTool().name(file.name).append(".img"));
                            _fsc->writeFileFromBuf(iNode, _buff, file.size);
                            _fsc->flushDisk();
                            cout << "successfully installed " << USBs[option]
                                 << ": " << file.name << endl;
                            trouble = false;
                            break;
                        }
                    }
                    if (trouble) {
                        cout <<"not detected!" << endl;
                        continue;
                    }
                    trouble = true;
                } else cout << "wrong input!" << endl;
            } else break;
        }
        cout << "checking installed operating system ... ";
        vector<INode> list = _fsc->dir();
        for (const auto &file: list) {
            if (FileTool().ext(file.name) == "img") {
                OSLocation = file.name;
                trouble = false;
                break;
            }
        }
        if (trouble) {
            cout << "not detected!" << endl;
            cout << "press any key to shutdown the machine >> ";
            getchar(); exit(0);
        } else {
            cout << "done!" << endl;
            trouble = true;
        }
        cout << "loading operating system ... " << endl;
        HINSTANCE hNewExe = ShellExecute(nullptr, "open", OSLocation.c_str(), nullptr, nullptr, SW_SHOW);
    }
};


#endif //EFS_BIOS_H
