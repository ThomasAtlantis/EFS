//
// Created by MAC on 2019/6/26.
//

#ifndef EFS_FORMAT_TOOL_H
#define EFS_FORMAT_TOOL_H

#include <utility>
#include "../VM/tiny_efs.h"

class FormatTool {
private:
    FSController * _fsc;
    VHDController * _vhdc;
    std::fstream _fileStream;
public:
    void format(string srcFile, string dstFile) {
        cout << "formatting ... ";
        _fileStream.open(dstFile, std::ios::out);
        _fileStream.close();
        _vhdc = new VHDController(std::move(dstFile));
        _fsc = new FSController(* _vhdc, OS_PART_BLOCK_SIZE, true);
        _fileStream.open(srcFile, std::ios::in| std::ios::binary);
        _fileStream.seekp(std::ios::end);
        auto size = static_cast<unsigned int>(_fileStream.tellp());
        auto * buff = new char[size];
        _fileStream.read(buff, size);
        _fileStream.close();
        INode iNode;
        _fsc->writeFileFromBuf(iNode, buff, size);
        _fsc->flushDisk();
        cout << "done" << endl;
    }
};

#endif //EFS_FORMAT_TOOL_H
