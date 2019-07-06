//
// Created by MAC on 2019/7/5.
//

#pragma once

#include "../FSC/fsc.h"
#include "../APP/app.h"
#include "../error.h"
#include "../user.h"
#include "part.h"

#define MAJR 0
#define VICE 1
#define PORT 2
#define INF 0
#define SYS 1
#define SHR 2
#define USR 3

class VFSController {
private:
    int & _curUser; // 当前用户
    vector<FSController *> _fsc; // 文件系统列表
    PartData _partData; // 分区信息
    const UserData &_userData;
    const vector<string> & _disk; // 硬盘路径
    const bid_t _sysPartMin, _sysPartMax; // 系统磁盘片的最小块号和最大块号
    const bid_t _sliceSize;       // 系统磁盘片大小（块数）
    VHDController _vhdc;

public:
    typedef FSController::INode INode;
    INode * curINode; // 当前路径i节点

    int curPart; // 当前分区

    VFSController(
        int &curUser, const UserData &userData,
        const vector<string> & disk, const bid_t sysPartMin,
        const bid_t sysPartMax, const bid_t sliceSize):
        _curUser(curUser), _userData(userData), _disk(disk), _vhdc(_disk[MAJR]),
        _sysPartMin(sysPartMin), _sysPartMax(sysPartMax), _sliceSize(sliceSize) {
    }

    INode * defaultRoot() {
        return _fsc[USR]->rootINode;
    }

    void install(App &app) {
        // 硬盘初始化
        _partData.diskCount = static_cast<int>(_disk.size());

        // 分区初始化::系统参数区
        _partData.partCount = 0;
        bid_t sizePointer = _sysPartMin;
        _setPart(_PARTNAME_INFO, _INFORMATION_SIZE, MAJR, sizePointer);

        // 分区初始化::系统区
        app.initPartInfo();
        app.showTotalSize(_sliceSize);
        app.showPartInfo(_PARTNAME_SYS, _PARTSIZE_SYS);
        _setPart(_PARTNAME_SYS, _PARTSIZE_SYS, MAJR, sizePointer);
        app.showPartSize(_PARTSIZE_SYS, _sliceSize);

        // 分区初始化::共享区
        app.showPartInfo(_PARTNAME_SHARE);
        #if DEBUG
        bid_t partSizeShare = 1024;
        #else
        bid_t partSizeShare = app.inputPartSize(_sliceSize);
        #endif
        _setPart(_PARTNAME_SHARE, partSizeShare, MAJR, sizePointer);
        app.showPartSize(partSizeShare, _sliceSize);

        // 分区初始化::用户区
        bid_t partSizeUser = _sliceSize - _PARTSIZE_SYS - partSizeShare;
        app.showPartInfo(_PARTNAME_USER, partSizeUser);
        _setPart(_PARTNAME_USER, partSizeUser, MAJR, sizePointer);
        app.showPartSize(partSizeUser, _sliceSize);

        // 操作系统参数写回硬盘
        _vhdc.writeBlock((char *) & _partData, _sysPartMin + _PART_DATA_OFFSET);
        _initFSC(); app.over();
        for (int i = 1; i < _partData.partCount; ++ i)
            _fsc[i]->_fbc.formatting();
    }

    void _setPart(const string &name, bid_t size, bid_t diskN, bid_t &sizePointer) {
        strcpy(_partData.partNames[_partData.partCount], name.c_str());
        _partData.partStart[_partData.partCount] = sizePointer;
        _partData.partSizes[_partData.partCount] = size;
        _partData.partDiskN[_partData.partCount] = diskN;
        sizePointer += _partData.partSizes[_partData.partCount];
        _partData.partCount ++;
    }

    void _initFSC() {
        for (int i = 1; i < _partData.partCount; ++ i) {
            _fsc.push_back(
                new FSController(_disk[_partData.partDiskN[i]],
                    _partData.partStart[i], _partData.partStart[i] + _partData.partSizes[i],
                    _partData.partNames[i], _SUPERADMIN_NAME)
            );
        }
    }

    /**
     * 解析路径，分发到子文件系统
     * @param path 路径名
     * @return -1：cannot access; 子系统编号; -2: curINode
     */
    int parsePart(string &path) {
        if (path.empty()) return -1;
        if (path[0] == '/') {
            size_t index = path.find('/', 1);
            string partName = path.substr(0, index - 1);
            for (int i = 0; i < _partData.partCount; ++ i) {
                if (_partData.partNames[i] == partName) {
                    if (index < path.length() - 1)
                        path = path.substr(index + 1, path.length() - index);
                    else path = "";
                    return i;
                }
            }
            return -1;
        } else return curPart;
    }

    // /home/admin/a.txt
    // /home/admin

    // ./home/admin/a.txt
    // home/admin
    INode * parsePath(int &partNum, string &fileName) {
        partNum = parsePart(fileName);
        if (partNum == -1) return nullptr;
        INode * iNode = _fsc[partNum]->rootINode;
        if (fileName[0] == '.') {
            iNode = curINode;
            fileName = fileName.substr(1, fileName.length() - 1);
        }
        return _fsc[partNum]->parsePath(iNode, fileName);
    }

    INode * createFile(Error &e, string fileName, string curUser) {
        size_t index = fileName.rfind('/');
        string path = fileName.substr(0, index);
        fileName = fileName.substr(index + 1, fileName.length() - index - 1);
        int partNum; INode * iNode = parsePath(partNum, path);
        if (!accessible(iNode, mode::write)) return nullptr;
        _fsc[partNum]->createFile(iNode, std::move(fileName), std::move(curUser));
    }
    bool accessible(INode * iNode, char mode) { // 判断用户是否具有对应的权限
        int attrGroup = getAttrGroup(getOwner(iNode), _curUser);
        return (iNode->mode[attrGroup] & mode);
    }

    string getOwner(INode * iNode) {
        return string(iNode->owner);
    }

    int getAttrGroup(string owner, int user) {
        if (owner == _userData.userNames[user]) return group::user;
        for (int i = 0; i < _userData.userCount; ++ i) {
            if (_userData.userNames[i] == owner) {
                if (_userData.userGroup[i] == _userData.userGroup[user]) {
                    return group::group;
                } else {
                    return group::others;
                }
            }
        }
        return -1;
    }

    string curUserName() {
        return string(_userData.userNames[_curUser]);
    }
};
