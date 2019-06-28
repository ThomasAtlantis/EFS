//
// Created by MAC on 2019/6/19.
//

#ifndef EFS_BIOS_H
#define EFS_BIOS_H

#include <cstdlib>
#include <windows.h>
#include <shellapi.h>
#include "tiny_efs.h"

#define DEBUG 0
#define FORMATTING 0
#define _CONFIGURE_OFFSET 0
#define _SLICE_BLOCK_SIZE 4096

// TODO: 在i节点中加入系统分片信息，并进行空余分片维护

class BIOS {
private:
    #pragma pack(1)
    typedef struct {
        bool installed;
    } Configure;
    #pragma pack()
    bool trouble;
    string majorDisk;
    string majorSlot;
    string USBSlot;
    FSController * _fsc, * _fsc_usb;
    VHDController * _vhdc, * _vhdc_usb;
    std::fstream _fileStream;
    char _buff[BIOS_BUF_BLOCK_SIZE * BLOCK_SIZE];
public:
    BIOS(): trouble(true), majorSlot("./SLOT_1/"), USBSlot("./SLOT_X/") {}
    void test() {
        _vhdc = new VHDController("./SLOT_2/");
        _fsc = new FSController(*_vhdc, OS_PART_BLOCK_SIZE);
        INode iNode;
        unsigned int length = 10;
        strcpy(_buff, string(R"(hello world!)").append("\0").c_str());
        _fsc->createFile(iNode, "hello.txt");
        _fsc->writeFileFromBuf(iNode, _buff, length);
        _fsc->flushDisk();
        _fsc->readFileToBuf(iNode.bid, _buff);
        cout << _buff << endl;
    }
    string run() {
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
        #if FORMATTING
        _fsc = new FSController(*_vhdc, OS_PART_BLOCK_SIZE, true);
        #else
        _fsc = new FSController(*_vhdc, OS_PART_BLOCK_SIZE);
        #endif
        while (true) {
            cout << "install operating system from USB? Y/N" << endl;
            #if DEBUG
            char op = 'Y';
            #else
            char op; cin >> op;
            #endif
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
                #if DEBUG
                size_t option = 0;
                #else
                size_t option; cin >> option;
                #endif
                if (option >= 0 && option < USBs.size()) {
                    _vhdc_usb = new VHDController(USBDisks[option]);
                    _fsc_usb = new FSController(*_vhdc_usb, OS_PART_BLOCK_SIZE);
                    cout << "scanning operating system mirrors ... ";
                    vector<INode> isoList = _fsc_usb->dir();
                    for (const auto &file: isoList) {
                        if (FileTool().ext(file.name) == "iso") {
                            cout << "installing ..." << endl;
                            INode iNode;
                            string imgFileName = FileTool().name(file.name).append(".img");
                            for (auto &osFile: _fsc->dir()) {
                                if (osFile.name == imgFileName) {
                                    _fsc->deleteFile(osFile);
                                    break;
                                }
                            }
                            auto * buff = new char [file.blocks * BLOCK_CONTENT_SIZE];
                            _fsc_usb->readFileToBuf(file.bid, buff);
                            _fsc->createFile(iNode, imgFileName);
                            _fsc->writeFileFromBuf(iNode, buff, file.size);
                            _fsc->flushDisk();
                            delete buff;
                            Configure configure;
                            configure.installed = false;
                            vector<INode> osFileNames;
                            for (const auto &file: _fsc->dir()) {
                                if (FileTool().ext(file.name) == "img") {
                                    osFileNames.emplace_back(file);
                                }
                            }
                            _vhdc->writeBlock((char *) & configure,
                                  osFileNames.size() * _SLICE_BLOCK_SIZE + _CONFIGURE_OFFSET);
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
            #if DEBUG
            break;
            #endif
        }
        cout << "checking installed operating system ... ";
        vector<INode> osFileNames;
        for (const auto &file: _fsc->dir()) {
            if (FileTool().ext(file.name) == "img") {
                osFileNames.emplace_back(file);
            }
        }
        if (osFileNames.empty()) {
            cout << "not detected!" << endl;
            cout << "press any key to shutdown the machine >> ";
            getchar(); exit(0);
        } else cout << "done!" << endl;
        size_t option;
        while (true) {
            cout << "select an operating system to start: " << endl;
            for (size_t i = 0; i < osFileNames.size(); ++ i)
                cout << "[" << i << "] " << osFileNames[i].name << endl;
            #if DEBUG
            option = 0;
            #else
            cin >> option;
            #endif
            if (option >= 0 && option < osFileNames.size()) break;
            else cout << "wrong input!" << endl;
        }
        cout << "loading operating system ... " << endl;
        cout << "decompressing " << osFileNames[option].name << " from disk ... ";
        string exeName = majorSlot + FileTool().name(osFileNames[option].name).append(".exe");
        if (_fsc->decompressEXE(exeName, osFileNames[option])) cout << "done" << endl;
        return exeName.append("-i "+ std::to_string(osFileNames.size() - 1 - option));
    }
};


#endif //EFS_BIOS_H
