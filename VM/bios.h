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
#include <ctime>
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
#define BLOCK_CONTENT_SIZE (BLOCK_SIZE-sizeof(bid_t))
#pragma pack (1)
typedef struct {
    bid_t freeMaxi; // 空闲块总数量
    bid_t freeHead; // 第一个空闲块
    bid_t dataHead; // 第一个数据块
//    char padding[BLOCK_SIZE - 2 * sizeof(bid_t)];
} SuperBlock;
typedef struct {
    bid_t blockNext;
    char data[BLOCK_CONTENT_SIZE];
} Block;
#pragma pack ()

class FBController { // 使用隐式链接分配
private:
    const size_t _blockSize; // 外存块大小
    const bid_t _blockMaxi; // 外存块总数量
    SuperBlock _superBlock; // 内存超级块缓冲区
    VHDController& _vhdc; // 通过引用vhdc调用VHD接口
public:

    FBController(VHDController& vhdc, bid_t blockMaxi, size_t blockSize = BLOCK_SIZE):
        _vhdc(vhdc), _blockSize(blockSize), _blockMaxi(blockMaxi - 1) {}

    bool formatting() {
        _superBlock.freeMaxi = _blockMaxi;
        _superBlock.freeHead = 1;
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
        bid_t sav = _superBlock.freeHead;
        _superBlock.freeHead = blockID;
        block.blockNext = sav;
        _vhdc.writeBlock((char *) & block, blockID);
        _superBlock.freeMaxi ++;
        return true;
    }

    bool distribute(bid_t & blockID) { // 分配一个空闲块，并将块号放到blockID中，若失败则返回false
        if (_superBlock.freeMaxi == 0) return false;
        Block block;
        blockID = _superBlock.freeHead;
        _vhdc.readBlock((char *) & block, blockID);
        _superBlock.freeHead = block.blockNext;
        _superBlock.freeMaxi --;
        return true;
    }

    SuperBlock& superBlock() {
        return _superBlock;
    }
};

struct INode { // sizeof(INode) = 128
    bid_t bid; // i节点所在的磁盘块号
    char name[FILENAME_MAXLEN + 1]; // 文件名
    unsigned int size; // 文件大小
    time_t atime; // 访问时间
    time_t mtime; // 修改时间
    unsigned int blocks; // 文件所占块数计数
    unsigned int bytes; // 最后一块的字节计数
    bid_t data; // 数据开始块号
    bid_t nextINode; // 下一个i节点
    INode() {
        memset(&this->bid, 0, sizeof(INode));
    }
};

class FSController { // 只支持创建和删除，不支持修改
private:
    VHDController& _vhdc; // 通过引用获得虚拟磁盘层的接口
    FBController _fbc;  // 空闲块管理模块
    const bid_t _blockMaxi;
public:
    FSController(VHDController& vhdc, bid_t blockMaxi):
        _vhdc(vhdc), _blockMaxi(blockMaxi), _fbc(vhdc, blockMaxi) {
        _fbc.loadSuperBLock();
    }
    // i节点操作
    bool getINodeByID(bid_t id, INode& iNode) {
        _vhdc.readBlock((char *) & iNode, id);
    }
    bool saveINodeByID(bid_t id, const INode& iNode) {
        _vhdc.writeBlock((char *) & iNode, id);
    }
    // TODO: 回归测试
    // 文件操作
    bool readFileToBuf(bid_t id, char* buf) { // 读文件到内存缓冲区
        INode iNode;
        if (!getINodeByID(id, iNode)) return false;
        if (iNode.blocks * BLOCK_CONTENT_SIZE > sizeof(buf)) return false;
        char * p = buf; bid_t blockID = iNode.data;
        Block block;
        for (int i = 0; i < iNode.blocks; ++ i) {
            _vhdc.readBlock((char *) & block, blockID);
            strcpy_s(p, BLOCK_CONTENT_SIZE, block.data);
            blockID = block.blockNext;
            p += BLOCK_CONTENT_SIZE;
        }
        return true;
    }
    bool createFile(INode& iNode, string fileName) {
        bid_t blockID;
        if (fileName.length() > FILENAME_MAXLEN ||
            !_fbc.distribute(blockID)) return false;
        iNode.mtime = iNode.atime = time(nullptr);
        strcpy_s(iNode.name, std::min(static_cast<size_t>(FILENAME_MAXLEN),
            fileName.length()), fileName.c_str());
        iNode.bid = blockID;
        iNode.nextINode = _fbc.superBlock().dataHead;
        _fbc.superBlock().dataHead = iNode.bid;
        return saveINodeByID(blockID, iNode);
    }
    // TODO: to be continue
    bool writeFileFromBuf(INode& iNode, char* buf, unsigned int length) { // 写内存缓冲区到文件
        // 文件iNode必须为新创建的
        iNode.size = length;
        iNode.blocks = length / BLOCK_CONTENT_SIZE;
        iNode.bytes = length % BLOCK_CONTENT_SIZE;
        bid_t preID, blockID;
        Block block;
        char * p = buf;
        if (!_fbc.distribute(preID)) return false;
        iNode.data = preID;
        for (int i = 0; i < iNode.blocks - 1; ++ i) {
            if (!_fbc.distribute(blockID)) return false;
            block.blockNext = blockID;
            strcpy_s(block.data, BLOCK_CONTENT_SIZE, p);
            p += BLOCK_CONTENT_SIZE;
            _vhdc.writeBlock((char *) & block, preID);
            preID = blockID;
        }
        strcpy_s(block.data, iNode.bytes, p);
        _vhdc.writeBlock((char *) & block, preID);
        return true;
    }
//    bool deleteFile(INode& iNode) {
//
//    }
//    vector<INode&> dir() {
//
//    }
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
