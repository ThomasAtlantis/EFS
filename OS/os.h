//
// Created by MAC on 2019/6/26.
//

#ifndef EFS_OS_H
#define EFS_OS_H

#include "FSC/fsc.h"
#include "APP/CLIController.h"

#define INSTALL_FLAG_PADDING (_BLOCK_SIZE \
    - sizeof(bool))
#define CONFIGURE_PADDING (_BLOCK_SIZE \
    - _MAX_PARTITIONS * _FILENAME_MAXLEN \
    - _MAX_USERS * (_USERNAME_MAXLEN + _PASSWORD_LENGTH + 1) \
    - sizeof(int) * (_MAX_PARTITIONS + 2))
class OS {
private:
    #pragma pack(1)
    typedef struct {
        bool installed;
        char padding[INSTALL_FLAG_PADDING];
    } InstallFlag;
    typedef struct {
        int userCount;
        int partCount;
        int partSizes[_MAX_PARTITIONS];
        char partNames[_MAX_PARTITIONS][_FILENAME_MAXLEN];
        char userNames[_MAX_USERS][_USERNAME_MAXLEN];
        char passwords[_MAX_USERS][_PASSWORD_LENGTH];
        char userGroup[_MAX_USERS];
        char padding[CONFIGURE_PADDING];
    } Configure;
    #pragma pack()
    bid_t _sysPartMin, _sysPartMax;
    string _majorDisk, _viceDisk, _portDisk; // 主硬盘、副硬盘、移动硬盘
    FSController *_fsc, *_fsc_share, *_fsc_usr;
    CLIController _cli;
    void * curINode;
    Configure configure;
    InstallFlag installFlag;
    int curUser;
public:
    OS(vector<string>& diskLocations, bid_t slice):
        _majorDisk(diskLocations[0]),
        _viceDisk(diskLocations[1]),
        _portDisk(diskLocations[2]),
        _sysPartMin(_OS_PART_BLOCK_SIZE + slice * _SLICE_BLOCK_SIZE),
        _sysPartMax(_sysPartMin + _PARTSIZE_SYS - 1) {
        _fsc = new FSController(_majorDisk, _sysPartMin + _INFORMATION_SIZE,
                _sysPartMax, _PARTNAME_SYS, _SUPERADMIN_NAME);
//        _fsc._vhdc.readBlock((char *) & installFlag, _sysPartMin + _INSTALL_FLAG_OFFSET);
        installFlag.installed = false;
        if (!installFlag.installed) {
            selfInstall();
            installFlag.installed = true;
            _fsc->_vhdc.writeBlock((char *) & installFlag, _sysPartMin + _INSTALL_FLAG_OFFSET);
        }
        selfInitialize();
        runShell();
    }
    void setUserSystem() {
        _fsc->setUserSystem(configure.userNames, configure.userGroup, configure.userCount, curUser);
        _fsc_share->setUserSystem(configure.userNames, configure.userGroup, configure.userCount, curUser);
        _fsc_usr->setUserSystem(configure.userNames, configure.userGroup, configure.userCount, curUser);
    }
    bool selfInitialize() {
        _fsc->_vhdc.readBlock((char *) & configure, _sysPartMin + _CONFIGURE_OFFSET);
        if (!_fsc_share) _fsc_share = new FSController(_majorDisk, _sysPartMax,
            _sysPartMax + configure.partSizes[1] - 1, _PARTNAME_SHARE, _SUPERADMIN_NAME);
        if (!_fsc_usr) _fsc_usr = new FSController(_majorDisk, _sysPartMax + configure.partSizes[1],
            _sysPartMax + configure.partSizes[1] + configure.partSizes[configure.partCount - 1], _PARTNAME_SYS, _SUPERADMIN_NAME);
        curINode = _fsc->rootINode;
        curUser = 0;
        setUserSystem();
        return true;
    }
    int _inputPartSize() {
        string text = _cli.inputText();
        int sysSize = std::stoi(text);
        while (sysSize < 0 || sysSize > _SLICE_BLOCK_SIZE) {
            for (int i = 0; i < text.length(); ++ i) cout << "\b \b";
            text = _cli.inputText();
            sysSize = std::stoi(text);
        }
        _showPartSize(sysSize);
        return sysSize;
    }
    void _showPartSize(int sysSize) {
        for (int i = 0; i < std::to_string(_SLICE_BLOCK_SIZE).length()
                - std::to_string(sysSize).length() + 4; ++ i)
            cout << " ";
        cout << "[";
        for (int i = 0; i < sysSize * 20 / _SLICE_BLOCK_SIZE; ++ i)
            cout << "-";
        cout << "]" << endl;
    }
    bool selfInstall() {
        configure.partCount = 0;
        cout << "Initialize disk partition ... " << endl;
        cout << "Total size of major disk: " << _SLICE_BLOCK_SIZE
             << " * " << _BLOCK_SIZE << " Bytes" << endl;
        cout << ">> " << std::left << std::setw(_FILENAME_MAXLEN)
             << _PARTNAME_SYS << "| block size: " << _PARTSIZE_SYS;
        strcpy(configure.partNames[configure.partCount], _PARTNAME_SHARE);
        configure.partSizes[configure.partCount ++] = _PARTSIZE_SYS;
        _showPartSize(_PARTSIZE_SYS);
        cout << ">> " << std::left << std::setw(_FILENAME_MAXLEN)
             << _PARTNAME_SHARE << "| block size: ";
        strcpy(configure.partNames[configure.partCount], _PARTNAME_SHARE);
        int partSizeShare = _inputPartSize();
        configure.partSizes[configure.partCount ++] = partSizeShare;
        cout << ">> " << std::left << std::setw(_FILENAME_MAXLEN)
             << _PARTNAME_USER << "| block size: " << (_SLICE_BLOCK_SIZE - _PARTSIZE_SYS - partSizeShare);
        strcpy(configure.partNames[configure.partCount], _PARTNAME_USER);
        configure.partSizes[configure.partCount ++] = _SLICE_BLOCK_SIZE - _PARTSIZE_SYS - partSizeShare;
        _showPartSize(configure.partSizes[configure.partCount - 1]);
        _fsc->_vhdc.writeBlock((char *) & configure, _CONFIGURE_OFFSET);
        _fsc_share = new FSController(_majorDisk, _sysPartMax,
            _sysPartMax + partSizeShare - 1, _PARTNAME_SHARE, _SUPERADMIN_NAME);
        _fsc_usr = new FSController(_majorDisk, _sysPartMax + partSizeShare,
            _sysPartMax + partSizeShare + configure.partSizes[configure.partCount - 1],
            _PARTNAME_SYS, _SUPERADMIN_NAME);
        cout << "done!" << endl << endl;

        configure.userCount = 0;
        cout << "Initialize user system ... " << endl;
        cout << "Initial super user: " << _SUPERADMIN_NAME << endl;
        cout << ">> Initial password: ";
        string password = _cli.inputPassword();
        while (true) {
            cout << ">> Repeat password: ";
            string repeat = _cli.inputPassword();
            if (repeat == password) break;
            else cout << "Wrong Repetition!" << endl;
        }
        strcpy(configure.userNames[configure.userCount], _SUPERADMIN_NAME);
        strcpy(configure.passwords[configure.userCount], password.c_str());
        configure.userGroup[configure.userCount] = 0;
        configure.userCount ++;
        _fsc->_vhdc.writeBlock((char *) & configure, _CONFIGURE_OFFSET);
        cout << "done" << endl << endl;
        return true;
    }
    bool runShell() {
        _cli.LoginSystem();
        while (true) {
            cout << "root@USER root/ " << endl;
            cout << "$ ";
            if (!_cli.ReadCommand()) break;
            cout << endl;
        }
        return true;
    }
    bool setUser(string name) {
        for (int i = 0; i < configure.userCount; ++ i) {
            if (configure.userNames[i] == name) {
                curUser = i;
                setUserSystem();
                return true;
            }
        }
        return false;
    }
};

#endif //EFS_OS_H
