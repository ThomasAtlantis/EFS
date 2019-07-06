//
// Created by MAC on 2019/6/26.
//

#ifndef EFS_OS_H
#define EFS_OS_H

#include "VFS/vfs.h"
#include "APP/app.h"
#include "user.h"

#define SU 0
#define INSTALL_FLAG_PADDING (_BLOCK_SIZE - sizeof(bool))

class OS {
private:
    // 标记系统是否已被安装
    #pragma pack(1)
    typedef struct {
        bool installed;
        char padding[INSTALL_FLAG_PADDING];
    } InstallFlag;
    #pragma pack()
    
    bid_t _sysPartMin, _sysPartMax; // 系统磁盘片的最小块号和最大块号
    bid_t _sliceSize;       // 系统磁盘片大小（块数）
    VHDController _vhdc;    // 读写OS配置信息的工具
    App _cli;               // 控制台用户界面
    vector<string> _disk;   // 虚拟硬盘路径
    VFSController * _vfs;   // 虚拟文件系统

public:
    typedef VFSController::INode INode;
    UserData userData;      // 系统参数
    InstallFlag installFlag;// 系统安装与否
    int curUser;            // 当前用户号
    INode * curINode;       // 当前路径i节点

    /**
     * 操作系统的构造函数
     * @param diskLocations 硬盘路径向量
     * @param sliceNum 系统总硬盘起始块号
     * @param sliceSize 系统总大小（块数）
     */
    OS(vector<string>& diskLocations, bid_t sliceStart, bid_t sliceSize):
        _sysPartMin(sliceStart), _sysPartMax(_sysPartMin + sliceSize - 1),
        _sliceSize(sliceSize), _vhdc(diskLocations[MAJR]) {
        _disk.insert(_disk.begin(), all(diskLocations));
        _vhdc.readBlock((char *) & installFlag, _sysPartMin + _INSTALL_FLAG_OFFSET);    // 读取安装标志
        if (!installFlag.installed) selfInstall(); else initialize();  // 如果系统未安装，安装系统
        _cli.run();
    }
    bool initialize() {
        curUser = SU;
        _vhdc.readBlock((char *) & userData, _sysPartMin + _USER_DATA_OFFSET);
        _vfs = new VFSController(curUser, userData, _disk, _sysPartMin, _sysPartMax, _sliceSize);
        _vfs->_initFSC(); curINode = _vfs->defaultRoot();
        return true;
    }

    bool selfInstall() {
        // 用户初始化::超级管理员
        userData.userCount = 0;
        string password = _cli.initPassword();
        strcpy(userData.userNames[userData.userCount], _SUPERADMIN_NAME);
        strcpy(userData.passwords[userData.userCount], password.c_str());
        userData.userGroup[userData.userCount ++] = 0;
        _vhdc.writeBlock((char *) & userData, _sysPartMin + _USER_DATA_OFFSET);
        _cli.over();

        // 分区初始化
        curUser = SU;
        _vfs = new VFSController(curUser, userData, _disk, _sysPartMin, _sysPartMax, _sliceSize);
        _vfs->install(_cli); curINode = _vfs->defaultRoot();

        // 改写安装标志位
        installFlag.installed = true;
        _vhdc.writeBlock((char *) & installFlag, _sysPartMin + _INSTALL_FLAG_OFFSET);
        return true;
    }
};

#endif //EFS_OS_H
