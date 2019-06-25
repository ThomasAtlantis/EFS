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
#define OS_PART_BLOCK_SIZE 1024
#define FILENAME_MAXLEN 20
#define BIOS_BUF_BLOCK_SIZE 1024

typedef size_t bid_t;

class VHDController {
private:
    string _fileName; // 虚拟磁盘的文件路径
    std::fstream _fileStream; // 虚拟磁盘文件流
    size_t _blockSize;
public:
    explicit VHDController(string fileName, size_t blockSize = BLOCK_SIZE):
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
        _fileStream.open(_fileName.c_str(), std::ios::in | std::ios::out| std::ios::binary);
        _fileStream.seekp(blockID * _blockSize, std::ios::beg);
        _fileStream.write(buffer, _blockSize * len);
        _fileStream.close();
        return !!_fileStream;
    }
};
#define BLOCK_CONTENT_SIZE (BLOCK_SIZE \
    - sizeof(bid_t))
#define SUPBLOCK_PADDING_SIZE (BLOCK_SIZE \
    - sizeof(bid_t) * 3)
#pragma pack (1)
typedef struct {
    bid_t freeMaxi; // 空闲块总数量
    bid_t freeHead; // 第一个空闲块
    bid_t dataHead; // 第一个数据块
    char padding[SUPBLOCK_PADDING_SIZE];
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
        _superBlock.dataHead = 0;
        Block block;
        memset(block.data, 0, sizeof(block.data));
        for (bid_t i = 2; i < _blockMaxi / 10; ++ i) {
            block.blockNext = i;
            _vhdc.writeBlock((char *) & block, i - 1);
            if (i % 100 == 0) cout << i << " / " << _blockMaxi << endl;
        }
        saveSuperBlock();
        return true;
    }

    bool loadSuperBlock() { // 加载超级块
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
#define INODE_PADDING_SIZE (BLOCK_SIZE \
    - sizeof(time_t) * 2 \
    - sizeof(unsigned int) * 3 \
    - sizeof(bid_t) * 4 \
    - FILENAME_MAXLEN - 1)
#pragma pack (1)
typedef struct { // sizeof(INode) = 128
    time_t atime; // 访问时间
    time_t mtime; // 修改时间
    unsigned int size; // 文件大小
    unsigned int blocks; // 文件所占块数计数
    unsigned int bytes; // 最后一块的字节计数
    bid_t bid; // i节点所在的磁盘块号
    bid_t data; // 数据开始块号
    bid_t nextINode; // 下一个i节点
    bid_t prevINode; // 上一个i节点
    char name[FILENAME_MAXLEN + 1]; // 文件名
    char padding[INODE_PADDING_SIZE];
} INode;
#pragma pack ()

class FSController { // 只支持创建和删除，不支持修改
private:
    VHDController& _vhdc; // 通过引用获得虚拟磁盘层的接口
    FBController _fbc;  // 空闲块管理模块
    const bid_t _blockMaxi;
public:
    FSController(VHDController& vhdc, bid_t blockMaxi):
        _vhdc(vhdc), _blockMaxi(blockMaxi), _fbc(vhdc, blockMaxi) {
        _fbc.formatting();
        _fbc.loadSuperBlock();
    }
    // i节点操作
    bool getINodeByID(bid_t id, INode& iNode) {
        return _vhdc.readBlock((char *) & iNode, id);
    }
    bool saveINodeByID(bid_t id, INode& iNode) {
        return _vhdc.writeBlock((char *) & iNode, id);
    }
    // 文件操作
    bool readFileToBuf(bid_t id, char* buf) { // 读文件到内存缓冲区
        INode iNode;
        if (!getINodeByID(id, iNode)) return false;
        if (iNode.blocks * BLOCK_CONTENT_SIZE > sizeof(buf)) return false;
        char * p = buf; bid_t blockID = iNode.data;
        Block block;
        for (int i = 0; i < iNode.blocks; ++ i) {
            _vhdc.readBlock((char *) & block, blockID);
            strncpy(p, block.data, BLOCK_CONTENT_SIZE);
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
        strncpy(iNode.name, fileName.c_str(),
            std::min(static_cast<size_t>(FILENAME_MAXLEN), fileName.length()));
        iNode.bid = blockID;
        iNode.nextINode = _fbc.superBlock().dataHead;
        iNode.prevINode = 0;
        if (iNode.nextINode != 0) {
            INode nextINode;
            _vhdc.readBlock((char *) & nextINode, iNode.nextINode);
            nextINode.prevINode = iNode.bid;
            _vhdc.writeBlock((char *) & nextINode, iNode.nextINode);
        }
        _fbc.superBlock().dataHead = iNode.bid;
        return saveINodeByID(blockID, iNode);
    }
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
            strncpy(block.data, p, BLOCK_CONTENT_SIZE);
            p += BLOCK_CONTENT_SIZE;
            _vhdc.writeBlock((char *) & block, preID);
            preID = blockID;
        }
        strncpy(block.data, p, iNode.bytes);
        _vhdc.writeBlock((char *) & block, preID);
        return true;
    }
    bool deleteFile(INode& iNode) {
        INode prevINode, nextINode;
        if (iNode.prevINode != 0) {
            _vhdc.readBlock((char *) & prevINode, iNode.prevINode);
            prevINode.nextINode = iNode.nextINode;
            _vhdc.writeBlock((char *) & prevINode, iNode.prevINode);
        } else {
            _fbc.superBlock().dataHead = iNode.nextINode;
            _fbc.saveSuperBlock();
        }
        if (iNode.nextINode != 0) {
            _vhdc.readBlock((char *) & nextINode, iNode.nextINode);
            nextINode.prevINode = prevINode.bid;
            _vhdc.writeBlock((char *) & nextINode, iNode.nextINode);
        }
        return true;
    }
    void flushDisk() {
        _fbc.saveSuperBlock();
    }
    vector<INode> dir() {
        INode iNode;
        vector<INode> list;
        for (bid_t blockID = _fbc.superBlock().dataHead; blockID != 0; blockID = iNode.nextINode) {
            _vhdc.readBlock((char *) & iNode, blockID);
            list.emplace_back(iNode);
        }
        return list;
    }
};

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
