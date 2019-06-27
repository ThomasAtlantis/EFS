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

#define _SUPBLOCK_PADDING_SIZE (_BLOCK_SIZE - \
    sizeof(bid_t) * (1 + _BLOCK_GROUP_SIZE))

class FBController { // 使用成组链接法管理空闲块
private:
    // 超级块的结构定义
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

    /**
     * 获取新块并初始化
     * @return 新块的指针
     */
    char * newBlock() {
        auto * block = new char [_BLOCK_SIZE];
        memset(block, 0, _BLOCK_SIZE); // 清0
        return block;
    }

    /**
     * 获取新超级块并初始化
     * @return 超级块指针
     */
    SuperBlock * newSuperBlock() {
        auto * sb = new SuperBlock;
        sb->freeCount = 0; // 栈顶指针归0
        for (auto & freeBlockID: sb->freeStack) // 栈内数据清0
            freeBlockID = 0;
        memset(sb->padding, 0, _SUPBLOCK_PADDING_SIZE);
        return sb;
    }

    /**
     * 空闲块栈压栈
     * @param sb 超级块
     * @param blockID 空闲块号
     */
    void push(SuperBlock * sb, bid_t blockID) {
        sb->freeStack[sb->freeCount] = blockID;
        sb->freeCount ++;
    }

    /**
     * 空闲块栈弹栈
     * @param sb 超级块
     * @return 空闲块号
     */
    bid_t pop(SuperBlock * sb) {
        sb->freeCount --;
        return sb->freeStack[sb->freeCount];
    }
public:
    /**
     * 构造函数，尝试从硬盘读取超级块
     * @param vhdc 虚拟硬盘管理器
     * @param minBlockID 管理的最小块号
     * @param maxBlockID 管理的最大块号
     */
    FBController(VHDController& vhdc,
        bid_t minBlockID,
        bid_t maxBlockID):
        _vhdc(vhdc),
        _minBlockID(minBlockID),
        _maxBlockID(maxBlockID) {
        _superBlock = newSuperBlock();
        loadSuperBlock();
    }

    /**
     * 格式化硬盘
     * @return 操作成功与否
     */
    bool formatting() {
        delete _superBlock;
        _superBlock = newSuperBlock();
        push(_superBlock, _minBlockID); // 保留最小块号作尾结束指针，同时也是储存超级块的位置
        for (bid_t blockID = _minBlockID + 1; blockID <= _maxBlockID; ++ blockID) {
            recycle(blockID); // 依次回收剩下的所有块
        }
        saveSuperBlock(); // 只要超级块有变化，刷到硬盘
        return true;
    }

    /**
     * 加载超级块
     * @return 操作成功与否
     */
    bool loadSuperBlock() {
        return _vhdc.readBlock((char *) _superBlock, _minBlockID); // 超级块存在最小块号处
    }

    /**
     * 保存超级块到VHD
     * @return 操作成功与否
     */
    bool saveSuperBlock() {
        return _vhdc.writeBlock((char *) _superBlock, _minBlockID); // 超级块存在最小块号处
    }

    /**
     * 回收blockID指定的块
     * @param blockID 待回收的块号
     * @return 操作成功与否
     */
    bool recycle(bid_t blockID) {
        if (_superBlock->freeCount < _BLOCK_GROUP_SIZE) { // 如果栈未满
            push(_superBlock, blockID); // 压栈
        } else { // 如果栈满
            _vhdc.writeBlock((char *) _superBlock, blockID); // 超级块写入空闲块，成为下级组长块
//            delete _superBlock; // 删除超级块，重新初始化为空
//            _superBlock = newSuperBlock();
            memset(&_superBlock->freeCount, 0, sizeof(SuperBlock));
            push(_superBlock, blockID); // 将下级组长块压栈
        }
        return true;
    }

    /**
     * 分配一个空闲块
     * @param blockID 存放分配的块号返回值
     * @return 操作成功与否
     */
    bool distribute(bid_t & blockID) {
        if (_superBlock->freeCount > 1) { // 如果栈中空闲块数大于1
            blockID = pop(_superBlock); // 弹栈
        } else if (_superBlock->freeStack[0] != _minBlockID) {
            blockID = pop(_superBlock); // 弹栈，释放下一级组长块
            _vhdc.readBlock((char *) _superBlock, blockID); // 把组长块写入到内存超级块缓冲区
        } else return false; // 如果栈剩余的1块为0号，说明磁盘已满
        return true;
    }

    /**
     * 超级块公有操作接口
     * @return 超级块的引用
     */
    SuperBlock * superBlock() {
        return _superBlock;
    }
};

#endif //EFS_FBC_H
