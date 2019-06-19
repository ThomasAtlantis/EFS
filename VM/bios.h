//
// Created by MAC on 2019/6/19.
//

#ifndef EFS_BIOS_H
#define EFS_BIOS_H

#include <cstdlib>
#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include <utility>
#include "..\utilities.h"

#define BLOCK_SIZE 1024
#define OS_PART_BLOCK_SIZE 10
#define FILENAME_MAXLEN 20

typedef size_t bid_t;

class VHDController {
private:
    string _fileName; // 虚拟磁盘的文件路径
    std::fstream _fileStream; // 虚拟磁盘文件流
    size_t _blockSize;
public:
    VHDController(string fileName, size_t blockSize = BLOCK_SIZE):
        _fileName(std::move(fileName)), _blockSize(blockSize) {}
    ~VHDController() = default;
    bool readBlock(char * buffer, bid_t blockID, int len = 1) { // 按块读
        _fileStream.open(_fileName.c_str(), std::ios::in | std::ios::binary);
        _fileStream.seekg(blockID * _blockSize, std::ios::beg);
        _fileStream.read(buffer, _blockSize * len);
        if (!_fileStream) return false;
        _fileStream.close();
        return true;
    }
    bool writeBlock(char * buffer, bid_t blockID, int len = 1) { // 按块写
        _fileStream.open(_fileName.c_str(), std::ios::out | std::ios::binary);
        _fileStream.seekp(blockID * _blockSize, std::ios::beg);
        _fileStream.write(buffer, _blockSize * len);
        if (!_fileStream) return false;
        _fileStream.close();
        return true;
    }
};

class FBController { // 使用隐式链接分配
private:
#pragma pack (1)
    typedef struct {
        bid_t freeMaxi; // 空闲块总数量
        bid_t blockNext;
        char padding[BLOCK_SIZE - 2 * sizeof(bid_t)];
    } SuperBlock;
    typedef struct {
        bid_t blockNext;
        char data[BLOCK_SIZE - sizeof(bid_t)];
    } Block;
#pragma pack ()
    const size_t _blockSize; // 外存块大小
    const bid_t _blockMaxi; // 外存块总数量
    SuperBlock _superBlock; // 内存超级块缓冲区
    VHDController& _vhdc; // 通过引用vhdc调用VHD接口
public:

    FBController(VHDController& vhdc, bid_t blockMaxi, size_t blockSize = BLOCK_SIZE):
        _vhdc(vhdc), _blockSize(blockSize), _blockMaxi(blockMaxi - 1) {
    }

    bool formatting() {
        _superBlock.freeMaxi = _blockMaxi;
        _superBlock.blockNext = 1;
        Block block;
        memset(block.data, 0, sizeof(block.data));
        for (bid_t i = 2; i < _blockMaxi; ++ i) {
            block.blockNext = i;
            _vhdc.writeBlock((char *) & block, i - 1);
        }
        saveSuperBlock();
        return true;
    }

    bool loadSuperBLock() { // 加载超级块
        return _vhdc.readBlock((char *) & _superBlock, 0);
    }

    bool saveSuperBlock() { // 保存超级块到VHD
        return _vhdc.writeBlock((char *) & _superBlock, 0);
    }

    bool recycle(bid_t blockID) { // 回收blockID指定的块
        Block block;
        _vhdc.readBlock((char *) & block, blockID);
        bid_t sav = _superBlock.blockNext;
        _superBlock.blockNext = blockID;
        block.blockNext = sav;
        _vhdc.writeBlock((char *) & block, blockID);
        _superBlock.freeMaxi ++;
        return true;
    }

    bool distribute(bid_t & blockID) { // 分配一个空闲块，并将块号放到blockID中，若失败则返回false
        if (_superBlock.freeMaxi == 0) return false;
        Block block;
        blockID = _superBlock.blockNext;
        _vhdc.readBlock((char *) & block, blockID);
        _superBlock.blockNext = block.blockNext;
        _superBlock.freeMaxi --;
        return true;
    }
};

struct INode { // sizeof(INode) = 128
    bid_t bid; // i节点所在的磁盘块号
    char name[FILENAME_MAXLEN]; // 文件名
    unsigned int size; // 文件大小
    time_t atime; // 访问时间
    time_t mtime; // 修改时间
    unsigned int blocks; // 文件所占块数计数
    unsigned short bytes; // 最后一块的字节计数
    unsigned int data; // 数据开始块号

    INode() {
        memset(&this->bid, 0, sizeof(INode));
    }
};

class FSController {
private:
    VHDController& _vhdc; // 通过引用获得虚拟磁盘层的接口
    FBController _fbc;  // 数据区空闲块管理模块
    FBController _ifbc; // i节点区空闲块管理模块
    const bid_t _blockMaxi;
public:
    FSController(VHDController& vhdc, bid_t blockMaxi):
        _vhdc(vhdc), _blockMaxi(blockMaxi), _fbc(vhdc, blockMaxi), _ifbc(vhdc, blockMaxi) {}
    // i节点操作
    bool getINodeByID(bid_t id, INode& iNode) {
        _vhdc.readBlock((char *) & iNode, id);
    }
    bool saveINodeByID(bid_t id, const INode& iNode) {
        _vhdc.writeBlock((char *) & iNode, id);
    }
    // 文件操作

    // TODO: 链式读写文件
    bool readFileToBuf(const INode& cur, int start, int len, char* buf) { // 读文件到内存缓冲区

    }
    bool appendBlocksToFile(INode& cur, int blockCnt) { // 追加空白块到文件尾

    }
    bool writeFileFromBuf(INode& cur, int start, int len, char* buf) { // 写内存缓冲区到文件

    }
    bool deleteFile(INode& cur) {

    }
};

class BIOS {
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
    BIOS(): trouble(true) {
        vector<string> files = FileTool().dir("./SLOT_1");
        _check("checking major hard disk", "not detected!", "done!", {
            for (const auto &fileName: files) {
                if (FileTool().ext(fileName) == "vhd") {
                    majorDisk = fileName;
                    trouble = false;
                    break;
                }
            }
        });

        // TODO: 查找、加载和安装操作系统

        _check("checking operating system", "not detected!", "done!", {
            for (const auto &fileName: files) {
                if (FileTool().ext(fileName) == "img") {
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


#endif //EFS_BIOS_H
