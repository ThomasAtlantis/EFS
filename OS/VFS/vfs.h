//
// Created by MAC on 2019/7/5.
//

#pragma once

#include "../FSC/fsc.h"
#include "../error.h"
#include "../user.h"
#include "part.h"

#define MAJR 0
#define VICE 1
#define PORT 2
#define SYS 0
#define SHR 1
#define USR 2
#define DEBUG 1

class VFSController {
private:
    int & _curUser; // 当前用户
    vector<FSController *> _fsc; // 文件系统列表
    PartData _partData; // 分区信息
    UserData &_userData;
    const vector<string> & _disk; // 硬盘路径
    const bid_t _sysPartMin, _sysPartMax; // 系统磁盘片的最小块号和最大块号
    const bid_t _sliceSize;       // 系统磁盘片大小（块数）
    VHDController * _vhdc;
    std::fstream * _fileStreamPool;

public:
    typedef FSController::INode INode;
    INode * curINode; // 当前路径i节点

    int curPart; // 当前分区

    VFSController(
        int &curUser, UserData &userData,
        const vector<string> & disk, const bid_t sysPartMin,
        const bid_t sysPartMax, const bid_t sliceSize, std::fstream * fileStreamPool):
        _curUser(curUser), _userData(userData), _disk(disk),
        _sysPartMin(sysPartMin), _sysPartMax(sysPartMax),
        _sliceSize(sliceSize), _fileStreamPool(fileStreamPool) {
        _vhdc = new VHDController(_disk[MAJR], _fileStreamPool[MAJR]);
        _vhdc->readBlock((char *) &_partData, _sysPartMin + _PART_DATA_OFFSET);
        curPart = USR;
    }

    void setDefaultCurINode() {
        curINode = _fsc[USR]->rootINode;
    }

    void install() {
        // 硬盘初始化
        _partData.diskCount = static_cast<int>(_disk.size());

        // 分区初始化::系统参数区
        _partData.partCount = 0;
        bid_t sizePointer = _sysPartMin + _INFORMATION_SIZE;

        // 分区初始化::系统区
        initPartInfo();
        showTotalSize(_sliceSize);
        showPartInfo(_PARTNAME_SYS, _PARTSIZE_SYS);
        _setPart(_PARTNAME_SYS, _PARTSIZE_SYS, MAJR, sizePointer);
        showPartSize(_PARTSIZE_SYS, _sliceSize);

        // 分区初始化::共享区
        showPartInfo(_PARTNAME_SHARE);
        #if DEBUG
        bid_t partSizeShare = 1024;
        #else
        bid_t partSizeShare = inputPartSize(_sliceSize);
        #endif
        _setPart(_PARTNAME_SHARE, partSizeShare, MAJR, sizePointer);
        showPartSize(partSizeShare, _sliceSize);

        // 分区初始化::用户区
        bid_t partSizeUser = _sliceSize - _PARTSIZE_SYS - partSizeShare;
        showPartInfo(_PARTNAME_USER, partSizeUser);
        _setPart(_PARTNAME_USER, partSizeUser, MAJR, sizePointer);
        showPartSize(partSizeUser, _sliceSize);

        // 操作系统参数写回硬盘
        _vhdc->writeBlock((char *) & _partData, _sysPartMin + _PART_DATA_OFFSET);
        _initFSC(true); cout << "done" << endl << endl;
    }

    void _setPart(const string &name, bid_t size, bid_t diskN, bid_t &sizePointer) {
        strcpy(_partData.partNames[_partData.partCount], name.c_str());
        _partData.partStart[_partData.partCount] = sizePointer;
        _partData.partSizes[_partData.partCount] = size;
        _partData.partDiskN[_partData.partCount] = diskN;
        sizePointer += (_partData.partSizes[_partData.partCount]);
        _partData.partCount ++;
    }

    void _initFSC(bool formatFlag=false) {
        for (int i = 0; i < _partData.partCount; ++ i) {
            _fsc.push_back(
                new FSController(_disk[_partData.partDiskN[i]],
                    _partData.partStart[i], _partData.partStart[i] + _partData.partSizes[i] - 1,
                    _partData.partNames[i], _SUPERADMIN_NAME, _fileStreamPool[_partData.partDiskN[i]], formatFlag)
            );
        }
    }

    void updateLoginTime() {
        _userData.loginTime = time(nullptr);
    }

    string getLoginTime() {
        std::stringstream ss;
        ss << std::put_time(std::localtime(&_userData.loginTime), "%Y-%m-%d %H:%M ");
        return ss.str();
    }

    void saveInfo() {
        for (int i = 0; i < _partData.partCount; ++ i) {
            _fsc[i]->_fbc.saveSuperBlock();
        }
        _vhdc->writeBlock((char *) &_userData, _sysPartMin + _USER_DATA_OFFSET);
        _vhdc->writeBlock((char *) &_partData, _sysPartMin + _PART_DATA_OFFSET);
    }

    INode * parsePath(int &partNum, string &fileName) {
        INode * iNode = nullptr;
        if (!fileName.empty() && fileName[0] == '/') {
            size_t index = fileName.find('/', 1);
            string partName = fileName.substr(0, index);
            for (int i = 0; i < _partData.partCount; ++i) {
                if (_partData.partNames[i] == partName) {
                    if (index != string::npos && index < fileName.length() - 1)
                        fileName = fileName.substr(index + 1, fileName.length() - index);
                    else fileName = "";
                    partNum = i;
                    iNode = _fsc[partNum]->rootINode;
                    break;
                }
            }
        } else {
            iNode = curINode;
            partNum = curPart;
            if (fileName == ".") fileName = "";
            else if (fileName == "..") {
                iNode = _fsc[partNum]->parentINode(iNode);
                fileName = "";
            } else if (fileName.substr(0, 2) == "./") {
                if (fileName.length() == 2) fileName = "";
                else fileName = fileName.substr(2, fileName.length() - 2);
            }
        }
        if (!iNode) return nullptr;
        else return _fsc[partNum]->parsePath(iNode, fileName);
    }

    INode * createFile(int &error, string fileName, string curUser) {
        size_t index = fileName.rfind('/');
        string path;
        if (index == string::npos) {
            path = "";
        } else if (index == 0) {
            error = -6;
            return nullptr;
        } else {
            path = fileName.substr(0, index);
            fileName = fileName.substr(index + 1, fileName.length() - index - 1);
        }
        error = fileNameCheck(fileName);
        if (error) return nullptr;
        int partNum; INode * iNode = parsePath(partNum, path);
        if (!iNode || !accessible(iNode, mode::write)) return nullptr;
        return _fsc[partNum]->createFile(error, iNode, std::move(fileName), std::move(curUser));
    }

    int fileNameCheck(const string &fileName) {
        if (fileName.empty()) return -2;
        if (fileName.length() > _FILENAME_MAXLEN) return -5;
        if (fileName[0] == '.') return -6;
        for (auto ch: fileName)
            if (ch == '/' || ch == ' ')
                return -6;
        return 0;
    }

    INode * createDir(int &error, string dirName, string curUser) {
        size_t index = dirName.rfind('/');
        string path;
        if (index == string::npos) {
            path = "";
        } else if (index == 0) {
            error = -6;
            return nullptr;
        } else {
            path = dirName.substr(0, index);
            dirName = dirName.substr(index + 1, dirName.length() - index - 1);
        }
        error = fileNameCheck(dirName);
        if (error) return nullptr;
        int partNum; INode * iNode = parsePath(partNum, path);
        if (!iNode || !accessible(iNode, mode::write)) return nullptr;
        return _fsc[partNum]->createDir(error, iNode, std::move(dirName), std::move(curUser));
    }

    bool removeFile(int &error, int partNum, INode * iNode) {
        error = 0;
        INode * parent = _fsc[partNum]->parentINode(iNode);
        if (!parent || !accessible(parent, mode::write)) {
            error = -4; return false;
        }
        int size = -iNode->size;
        _fsc[partNum]->changeParentSize(iNode, size);
        bool result =  _fsc[partNum]->removeFile(*iNode);
        _fsc[curPart]->_vhdc.readBlock((char *) curINode, curINode->bid);
        return result;
    }

    vector<INode *> listDir(int &error, string path, bool allFlag=false) {
        error = 0;
        int partNum; INode * iNode = parsePath(partNum, path);
        if (!iNode) { error = -3; return {}; }
        if (!accessible(iNode, mode::read)) { error = -4; return {};}
        if (iNode->type == 'F') return {iNode};
        else return _fsc[partNum]->listDir(*iNode, allFlag);
    }

    // -3: not exists; -4: no right
    bool changeDir(int &error, string path) {
        if (path == ".") return true;
        int partNum; INode * iNode = parsePath(partNum, path);
        if (!iNode) { error = -3; return false; }
        if (!accessible(iNode, mode::read)) { error = -4; return false; }
        _vhdc->writeBlock((char *) curINode, curINode->bid);
        curINode = iNode;
        curPart = partNum;
        return true;
    }

    bool accessible(INode * iNode, char mode) { // 判断用户是否具有对应的权限
        int attrGroup = getAttrGroup(getOwner(iNode), _curUser);
        return (iNode->mode[attrGroup] & mode);
    }

    Buffer * readFile(int &error, string fileName) {
        error = 0;
        int partNum; INode * iNode = parsePath(partNum, fileName);
        if (!iNode) { error = -3; return nullptr; }
        if (!accessible(iNode, mode::read)) { error = -4; return nullptr;}
        return _fsc[partNum]->readFile({iNode, fio::in});
    }

    bool writeFile(int &error, string fileName, Buffer buffer) {
        error = 0;
        int partNum; INode * iNode = parsePath(partNum, fileName);
        if (!iNode) { error = -3; return false; }
        if (!accessible(iNode, mode::write)) { error = -4; return false;}
        return _fsc[partNum]->writeFile({iNode, fio::out}, buffer);
    }

    string workingDir(int partNum, INode * iNode) {
        string path;
        _fsc[partNum]->workingDir(path, iNode);
        return path;
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

    bool matchPassword(const string &userName, const string &password) {
        for (int i = 0; i < _userData.userCount; ++ i) {
            if (_userData.userNames[i] == userName) {
                return _userData.passwords[i] == password;
            }
        }
        return false;
    }

    char groupOf(const string &user) {
        for (int i = 0; i < _userData.userCount; ++ i) {
            if (_userData.userNames[i] == user) {
                return _userData.userGroup[i];
            }
        }
        return '-';
    }

    // 一些GUI界面，本应在APP类中，但产生了互相依赖
    // 此处应该使用依赖倒转，APP类单向调用VFS，时间不够先凑活一下
    void showTotalSize(bid_t sliceSize) {
        cout << "Total size of major disk: " << sliceSize
             << " * " << _BLOCK_SIZE << " Bytes" << endl;
    }
    void initPartInfo() {
        cout << "Initialize disk partition ... " << endl;
    }
    void showPartInfo(const string &partName, bid_t partSize = 0) {
        cout << ">> " << std::left << std::setw(_FILENAME_MAXLEN)
             << partName << "| block size: ";
        if (partSize != 0) cout << partSize;
    }
    void showPartSize(const bid_t &sysSize, const bid_t &_sliceSize) {
        for (int i = 0; i < std::to_string(_sliceSize).length()
                            - std::to_string(sysSize).length() + 4; ++ i) cout << " ";
        cout << "[";
        for (int i = 0; i < sysSize * 20 / _sliceSize; ++ i)
            cout << "-";
        cout << "]" << endl;
    }
    string inputText(char limit = 'N') {
        int ch; string text {};
        while (true) {
            ch = _getch();
            if (ch == '\b') {
                if (text.length() > 0) {
                    text = text.substr(0, text.length() - 1);
                    cout << "\b \b";
                }
            } else if (ch == '\r') {
                return text;
            } else if (limit == 'N' && ch <= '9' && ch >= '0') {
                text += static_cast<char>(ch);
                cout << static_cast<char>(ch);
            }
        }
    }
    bid_t inputPartSize(const bid_t &_sliceSize) {
        string text = inputText();
        auto sysSize = static_cast<bid_t>(std::stoi(text));
        while (sysSize <= 0 || sysSize > _sliceSize) {
            for (int i = 0; i < text.length(); ++ i) cout << "\b \b";
            text = inputText();
            sysSize = static_cast<bid_t>(std::stoi(text));
        }
        showPartSize(sysSize, _sliceSize);
        return sysSize;
    }
};
