//
// Created by MAC on 2019/6/26.
//

#ifndef EFS_VHD_H
#define EFS_VHD_H

#include "..\..\utilities.h"
#include "..\params.h"

class VHDController {
private:
    string _fileName; // 虚拟磁盘的文件路径
    std::fstream &_fileStream; // 虚拟磁盘文件流
    size_t _blockSize; // 块大小
    size_t _flushCount;
public:
    /**
     * 构造函数，打开文件二进制读写流
     * @param fileName 虚拟磁盘的文件**相对**路径
     * @param blockSize 块大小，默认为宏定义值
     */
    explicit VHDController(string fileName, std::fstream &fileStream, size_t blockSize = _BLOCK_SIZE):
            _fileName(std::move(fileName)), _fileStream(fileStream), _blockSize(blockSize) {
        _flushCount = 1;
    }
    /**
     * 读磁盘块
     * @param buffer 存储读出的数据的缓冲区
     * @param blockID 要读的块号
     * @param len 连续读的块数
     * @return 是否成功
     */
    bool readBlock(char * buffer, bid_t blockID, int len = 1) { // 按块读
        _fileStream.seekg(blockID * _blockSize, std::ios::beg); // 移动文件流指针到指定位置
        if (_flushCount % 100 == 0 || _fileStream.tellg() < 0) { // [IMPORTANT] 缓冲区溢出时需要重新打开文件以刷新缓冲区
            _fileStream.flush();
            _fileStream.seekg(blockID * _blockSize, std::ios::beg);
        }
        _fileStream.read(buffer, _blockSize * len);
        _flushCount = (_flushCount + 1) % 10000000000;
        return true;
    }
    /**
     * 写磁盘块
     * @param buffer 存储要写的数据的缓冲区
     * @param blockID 要写的块号
     * @param len 连续写的块数
     * @return 是否成功
     */
    bool writeBlock(char * buffer, bid_t blockID, int len = 1) { // 按块写
        _fileStream.seekp(blockID * _blockSize, std::ios::beg); // 移动文件流指针到指定位置
        if (_flushCount % 100 == 0 || _fileStream.tellp() < 0) { // [IMPORTANT] 缓冲区溢出时需要重新打开文件以刷新缓冲区
            _fileStream.flush();
//            _fileStream.close();
//            _fileStream.open(_fileName, std::ios::in| std::ios::out| std::ios::binary);
            _fileStream.seekp(blockID * _blockSize, std::ios::beg);
        }
        bool tmp = _fileStream.is_open();
        auto x = static_cast<int>(_fileStream.tellp());
        _fileStream.write(buffer, _blockSize * len);
        _flushCount = (_flushCount + 1) % 10000000000;
        return !!_fileStream;
    }
};

#endif //EFS_VHD_H
