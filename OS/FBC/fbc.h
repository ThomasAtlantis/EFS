//
// Created by MAC on 2019/6/26.
//

#ifndef EFS_FBC_H
#define EFS_FBC_H

#include <fstream>
#include <utility>
#include <ctime>
#include "..\..\utilities.h"
#include "..\VHD\vhd.h"
#include "..\params.h"

#define _BLOCK_GROUP_SIZE 100

#define _SUPBLOCK_PADDING_SIZE (_BLOCK_SIZE - \
    sizeof(bid_t) * (1 + _BLOCK_GROUP_SIZE))

class FBController { // 使用成组链接法管理空闲块
private:
    #pragma pack (1)
    typedef struct {
        bid_t freeCount; // 空闲块总数量
        bid_t freeStack[_BLOCK_GROUP_SIZE]; // 空闲块栈
        char padding[_SUPBLOCK_PADDING_SIZE];
    } SuperBlock;
    #pragma pack ()
    const bid_t _maxBlockID; // 受管理的最大块号
    const bid_t _minBlockID; // 受管理的最小块号
    SuperBlock * _superBlock; // 内存超级块缓冲区
    VHDController& _vhdc; // 通过引用vhdc调用VHD接口
    char * newBlock() {
        auto * block = new char [_BLOCK_SIZE];
        memset(block, 0, _BLOCK_SIZE);
        return block;
    }
    SuperBlock * newSuperBlock() {
        auto * sb = new SuperBlock;
        sb->freeCount = 0;
        for (auto & freeBlockID: sb->freeStack)
            freeBlockID = 0;
        memset(sb->padding, 0, _SUPBLOCK_PADDING_SIZE);
        return sb;
    }
    void push(SuperBlock * sb, bid_t blockID) {
        sb->freeStack[sb->freeCount] = blockID;
        sb->freeCount ++;
    }
    bid_t pop(SuperBlock * sb) {
        sb->freeCount --;
        return sb->freeStack[sb->freeCount];
    }
public:
    FBController(VHDController& vhdc, bid_t maxBlockID, bid_t minBlockID):
        _vhdc(vhdc), _maxBlockID(maxBlockID), _minBlockID(minBlockID) {
        loadSuperBlock();
    }

    bool formatting() {
        _superBlock = newSuperBlock();
        push(_superBlock, 0);
        for (bid_t blockID = _minBlockID + 1; blockID <= _maxBlockID; ++ blockID)
            recycle(blockID);
        saveSuperBlock();
        return true;
    }

    bool loadSuperBlock() { // 加载超级块
        return _vhdc.readBlock((char *) & _superBlock, _minBlockID);
    }

    bool saveSuperBlock() { // 保存超级块到VHD
        return _vhdc.writeBlock((char *) & _superBlock, _minBlockID);
    }

    bool recycle(bid_t blockID) { // 回收blockID指定的块
        if (_superBlock->freeCount < _BLOCK_GROUP_SIZE) {
            push(_superBlock, blockID);
        } else {
            _vhdc.writeBlock((char *) _superBlock, blockID);
            delete _superBlock;
            _superBlock = newSuperBlock();
            push(_superBlock, blockID);
        }
        return true;
    }

    bool distribute(bid_t & blockID) { // 分配一个空闲块，并将块号放到blockID中，若失败则返回false
        if (_superBlock->freeCount > 1) {
            blockID = pop(_superBlock);
        } else if (_superBlock->freeStack[0] != 0) {
            blockID = pop(_superBlock);
            _vhdc.readBlock((char *) _superBlock, blockID);
        } else return false;
        return true;
    }

    SuperBlock * superBlock() {
        return _superBlock;
    }
};

#endif //EFS_FBC_H
