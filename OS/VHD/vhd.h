//
// Created by MAC on 2019/6/26.
//

#ifndef EFS_VHD_H
#define EFS_VHD_H


#include <fstream>
#include <utility>
#include <ctime>
#include "..\..\utilities.h"
#include "..\params.h"

typedef size_t bid_t;

class VHDController {
private:
    string _fileName; // 虚拟磁盘的文件路径
    std::fstream _fileStream; // 虚拟磁盘文件流
    size_t _blockSize;
public:
    explicit VHDController(string fileName, size_t blockSize = BLOCK_SIZE):
            _fileName(std::move(fileName)), _blockSize(blockSize) {
        _fileStream.open(_fileName.c_str(), std::ios::in | std::ios::out| std::ios::binary);
    }
    ~VHDController() {
        _fileStream.close();
    }
    bool readBlock(char * buffer, bid_t blockID, int len = 1) { // 按块读
        _fileStream.seekg(blockID * _blockSize, std::ios::beg);
        _fileStream.read(buffer, _blockSize * len);
        return true;
    }
    bool writeBlock(char * buffer, bid_t blockID, int len = 1) { // 按块写
        _fileStream.seekp(blockID * _blockSize, std::ios::beg);
        _fileStream.write(buffer, _blockSize * len);
        return !!_fileStream;
    }
};

#endif //EFS_VHD_H
