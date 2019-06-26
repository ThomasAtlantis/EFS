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
    explicit FormatTool(string diskName) {
        cout << "formatting ... ";
        _fileStream.open(diskName, std::ios::out);
        _fileStream.close();
        _vhdc = new VHDController(std::move(diskName));
        _fsc = new FSController(* _vhdc, OS_PART_BLOCK_SIZE, true);
        cout << "done" << endl;
    }
    void compress(string fileName) {
        cout << "compressing " << fileName << " into disk ... ";
        _fileStream.open(fileName, std::ios::in| std::ios::binary);
        if (! _fileStream) {
            cout << "failed to access " << fileName << endl;
            return;
        }
        _fileStream.seekg(0, std::ios::end);
        auto size = static_cast<unsigned int>(_fileStream.tellg());
        auto * buff = new char[size];
//        _fileStream.close();
//        _fileStream.open(fileName, std::ios::in| std::ios::binary);
        _fileStream.seekg(0, std::ios::beg);
        _fileStream.read(buff, size);
        _fileStream.close();
        INode iNode;
        _fsc->createFile(iNode, FileTool().name(std::move(fileName)).append(".iso"));
        _fsc->writeFileFromBuf(iNode, buff, size);
        _fsc->flushDisk();
        delete buff;
        cout << "done" << endl;
    }
    void decompress(const string &fileName) {
        cout << "decompressing " << fileName << " from disk ... ";
        bool fileFound = false;
        vector<INode> tmp = _fsc->dir();
        for (const auto &iNode: _fsc->dir()) {
            if (string(iNode.name) == fileName) {
                fileFound = true;
                auto * buff = new char[iNode.blocks * BLOCK_CONTENT_SIZE];
                _fsc->readFileToBuf(iNode.bid, buff);
                _fileStream.open("decompress.exe", std::ios::out);
                _fileStream.close();
                _fileStream.open("decompress.exe", std::ios::in| std::ios::out| std::ios::binary);
                _fileStream.write(buff, iNode.size);
                _fileStream.close();
                cout << "done" << endl;
                break;
            }
        }
        if (!fileFound) cout << fileName << " not found" << endl;
    }
};

#endif //EFS_FORMAT_TOOL_H
