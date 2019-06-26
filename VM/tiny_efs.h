//
// Created by MAC on 2019/6/26.
//

#ifndef EFS_TINY_EFS_H
#define EFS_TINY_EFS_H

#include <fstream>
#include <utility>
#include <ctime>
#include "..\utilities.h"

#define BLOCK_SIZE 1024
#define OS_PART_BLOCK_SIZE 4096
#define FILENAME_MAXLEN 20
#define BIOS_BUF_BLOCK_SIZE 1024
#define FORMAT_BUFFER_SIZE 100

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
        /* 未优化版本
        Block block;
        memset(block.data, 0, sizeof(block.data));
        for (bid_t i = 2; i < _blockMaxi; ++ i) {
            block.blockNext = i;
            _vhdc.writeBlock((char *) & block, i - 1);
            if (i % 100 == 0) cout << i << " / " << _blockMaxi << endl;
        }
        block.blockNext = 0;
        _vhdc.writeBlock((char *) & block, _blockMaxi - 1);
        */
        Block block[FORMAT_BUFFER_SIZE];
        for (auto &i : block) memset(i.data, 0, BLOCK_CONTENT_SIZE);
        bid_t div = _blockMaxi / FORMAT_BUFFER_SIZE;
        bid_t mod = _blockMaxi % FORMAT_BUFFER_SIZE;
        for (bid_t i = 0; i < div; ++ i) {
            for (int j = 0; j < FORMAT_BUFFER_SIZE; ++ j)
                block[j].blockNext = i * FORMAT_BUFFER_SIZE + j + 1;
            _vhdc.writeBlock((char *) & block[0], i * FORMAT_BUFFER_SIZE, FORMAT_BUFFER_SIZE);
        }
        if (mod != 0) {
            for (int j = 0; j < mod; ++ j)
                block[j].blockNext = _blockMaxi - mod + j + 1;
            _vhdc.writeBlock((char *) & block[0], _blockMaxi - mod, static_cast<int>(mod));
        }
        block[0].blockNext = 0;
        _vhdc.writeBlock((char *) & block[0], _blockMaxi - 1);
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
    FSController(VHDController& vhdc, bid_t blockMaxi, bool format = false):
        _vhdc(vhdc), _blockMaxi(blockMaxi), _fbc(vhdc, blockMaxi) {
        if (format) _fbc.formatting();
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
    bool readFileToBuf(bid_t id, char* buf, unsigned int length = static_cast<unsigned int>(-1)) { // 读文件到内存缓冲区
        INode iNode;
        if (!getINodeByID(id, iNode)) return false;
        if (iNode.blocks * BLOCK_CONTENT_SIZE > length) return false;
        char * p = buf; bid_t blockID = iNode.data;
        Block block;
        for (int i = 0; i < iNode.blocks; ++ i) {
            _vhdc.readBlock((char *) & block, blockID);
            BufferTool().copy(p, block.data, BLOCK_CONTENT_SIZE);
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
        size_t length = std::min(static_cast<size_t>(FILENAME_MAXLEN), fileName.length());
        BufferTool().copy(iNode.name, fileName.c_str(), length);
        iNode.name[length] = '\0';
        iNode.bid = blockID;
        iNode.nextINode = _fbc.superBlock().dataHead;
        iNode.prevINode = 0;
        iNode.data = 0;
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
        if (iNode.bytes > 0) iNode.blocks ++;
        bid_t preID, blockID;
        Block block;
        char * p = buf;
        if (!_fbc.distribute(preID)) return false;
        iNode.data = preID;
        for (int i = 0; i < iNode.blocks - 1; ++ i) {
            if (!_fbc.distribute(blockID)) return false;
            block.blockNext = blockID;
            BufferTool().copy(block.data, p, BLOCK_CONTENT_SIZE);
            p += BLOCK_CONTENT_SIZE;
            _vhdc.writeBlock((char *) & block, preID);
            preID = blockID;
        }
        BufferTool().copy(block.data, p, iNode.bytes);
        _vhdc.writeBlock((char *) & block, preID);
        return saveINodeByID(iNode.bid, iNode);
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

#endif //EFS_TINY_EFS_H
