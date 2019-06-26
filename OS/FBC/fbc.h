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

#endif //EFS_FBC_H
