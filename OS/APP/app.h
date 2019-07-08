//
// Created by Bessie on 2019/6/25.
//

#pragma once

#include "../../utilities.h"
#include "../VFS/vfs.h"
#include "../FSC/fsc.h"
#include "screen.h"

#define DEBUG 1

using std::ifstream;
using std::ofstream;

class App {

private:
    typedef FSController::INode INode;
    typedef struct {
        string helpDoc;
        bool (App::*func)(vector<string>&);
    } Command;
    VFSController &_vfs; // 虚拟文件系统
    Screen _screen;
    string _curPath = "~";
    string _symbol = "#";
    string _machineName = "@VirtualMachine";
    map<string, Command> _cmds;
    string _cmdLine;

public:
    App(VFSController &vfs): _vfs(vfs) {
        bind("help", help, "help: help <command>\n    Display information about builtin commands.");
        bind("touch", touch, "touch: touch <fileName>\n    Create a new empty file.");
        bind("ld", listDir, "ld: ld [-s|-l] [dirName]\n    list files in a directory.");
        bind("clear", clear, "clear: clear\n    clear the screen.");
        bind("rm", remove, "rm: rm [-r][-f] <fileName>\n    remove a file.");
        bind("mkdir", makeDir, "mkdir: mkdir <dirName>\n    create a new empty directory.");
    }

    bool run() {
//        freopen("data.in", "r", stdin);
//        freopen("data.out", "w", stdout);
        if (login()) {
            do {
                commandTip();
            } while (readCommand());
            _vfs.saveInfo();
            return true;
        } else return false;
    }

    void commandTip() {
        _screen.cprintf(_vfs.curUserName().c_str(), FOREGROUND_GREEN| FOREGROUND_INTENSITY);
        cout << _machineName << ":" << _curPath << _symbol << " ";
    }

    void bind(const string &cmdName, bool (App::*func)(vector<string>&), string helpDoc="") {
        Command cmd {std::move(helpDoc), func};
        _cmds[cmdName] = cmd;
    }

    bool readCommand() {
        fflush(stdin);
        getline(cin, _cmdLine);
        vector<string> cmdParam = stringTool().split(_cmdLine, " ");
        string cmdName = cmdParam[0];
        cmdParam.erase(cmdParam.begin());
        if (cmdName == "exit") return false;
        auto it = _cmds.find(cmdName);
        if (it == _cmds.end())
            cout << cmdName << ": command not found" << endl;
        else (this->*(it->second.func))(cmdParam);
        return true;
    }

    bool login() {
        cout << "login as: ";
        string userName, password;
//        cin >> userName;
//        cout << userName << _machineName << "'s password: ";
//        password = _inputPassword(_PASSWORD_LENGTH);
        userName = "root";
        password = "123456";
//        fflush(stdin);
        if (!_vfs.matchPassword(userName, password)) return false;
        cout << "Last login: " << _vfs.getLoginTime() << endl;
        cout << "Welcome to VirtualMachine X!" << endl;
        _vfs.updateLoginTime();
        return true;
    }

    string missParam(string cmd, const string &paramName) {
        string result = cmd + ": missing param " + paramName
            + "\nTry 'help " + cmd + "' for more information.";
        return result;
    }

    bool touch(vector<string> &param) {
        if (param.empty()) {
            cout << missParam("touch", "<fileName>") << endl;
            return false;
        }
        int error = 0;
        string fileName = param[0];
        if (!_vfs.createFile(error, fileName, _vfs.curUserName())) {
            cout << "touch: cannot create '" << param[0] << "': ";
            if (error == -3) cout << "File already exists";
            cout << endl;
            return false;
        }
        return true;
    }

    bool clear(vector<string> &param) {
        system("cls");
        return true;
    }

    bool makeDir(vector<string> &param) {
        if (param.empty()) {
            cout << missParam("mkdir", "<dirName>") << endl;
            return false;
        }
        int error = 0;
        string fileName = param[0];
        if (!_vfs.createDir(error, fileName, _vfs.curUserName())) {
            cout << "touch: cannot create '" << param[0] << "': ";
            if (error == -3) cout << "File already exists";
            cout << endl;
            return false;
        }
        return true;
        return true;
    }

    bool changeDir(vector<string> &param) {
        return true;
    }

    bool pwd(vector<string> &param) {
        return true;
    }

    bool moveFile(vector<string> &param) {
        return true;
    }

    bool copyFile(vector<string> &param) {
        return true;
    }

    bool remove(vector<string> &vec) {
        string fileName = _parseArgum(vec);
        if (fileName.empty()) {
            cout << missParam("touch", "<fileName>") << endl;
            return false;
        }
        string params = _parseParam(vec);
        bool confirmFlag = true;
        bool recursive = false;
        for (auto p: params) {
            if (p == 'f') confirmFlag = false;
            if (p == 'r') recursive = true;
        }
        return _remove(fileName, confirmFlag, recursive);
    }

    bool _remove(string fileName, bool confirmFlag, bool recursive) {
        int partNum;
        INode * iNode = _vfs.parsePath(partNum, fileName);
        if (!iNode) {
            cout << "rm: cannot remove '" << fileName << "': No such file or directory" << endl;
            return false;
        }
        if (iNode->type == 'D' && !recursive) {
                cout << "rm: cannot remove '" << fileName << "': Is a directory" << endl;
                return false;
        }
        if (confirmFlag) {
            if (iNode->type == 'F') {
                cout << "rm: remove regular file '" << fileName << "'?(y/n) ";
            } else if (iNode->type == 'D') {
                cout << "rm: remove directory '" << fileName << "'?(y/n) ";
            }
            char option;
            do {
                cin >> option;
            } while (option != 'y' && option != 'n');
            fflush(stdin);
            if (option == 'n') return false;
        }
        return _vfs.removeFile(partNum, iNode);
    }

    void _sortByName(vector<INode *> &list) {
        for (int i = 0; i < list.size(); ++ i) {
            for (int j = i + 1; j < list.size(); ++ j) {
                if (strcmp(list[i]->name, list[j]->name) > 0) {
                    INode * tmp = list[i];
                    list[i] = list[j];
                    list[j] = tmp;
                }
            }
        }
    }

    // 默认为短形式
    bool listDir(vector<string> &param) {
        string dirName = ".";
        bool longFlag = false;
        if (!param.empty() && param[0] == "-l") {
            longFlag = true;
        }
        if (param.size() == 1 && param[0] != "-l" && param[0] != "-s") {
            dirName = param[0];
        } else if (param.size() > 1) {
            dirName = param[1];
        }
        vector<INode *> list = _vfs.listDir(dirName);
        _sortByName(list);
        if (longFlag) {
            for (auto iNode: list) {
                cout << lower(iNode->type);
                for (char i : iNode->mode) {
                    if (i & mode::read) cout <<  "r"; else cout << "-";
                    if (i & mode::write) cout << "w"; else cout << "-";
                    if (i & mode::exec) cout << "x"; else cout << "-";
                }
                cout << " ";
                if (strlen(iNode->owner) > 6) cout << string(iNode->owner).substr(0, 6);
                else cout << std::setw(6) << iNode->owner;
                cout << " " << _vfs.groupOf(iNode->owner) << " ";
                cout << std::setw(6) << iNode->size << "KB ";
                std::stringstream ss;
                ss << std::put_time(std::localtime(&iNode->ctime), "%Y-%m-%d %H:%M ");
                cout << ss.str();
                if (iNode->type == 'D') _screen.cprintf(iNode->name, FOREGROUND_BLUE| FOREGROUND_INTENSITY);
                else cout << iNode->name;
                cout << endl;
            }
        } else {
            int cnt = 1;
            for (auto iNode: list) {
                if (iNode->type == 'D') _screen.cprintf(iNode->name, FOREGROUND_BLUE| FOREGROUND_INTENSITY);
                else cout << iNode->name;
                cout << "\t";
                if (cnt++ % 8 == 0) cout << endl;
            }
            if (cnt % 8 != 1 && !list.empty()) cout << endl;
        }
        return true;
    }

    bool help(vector<string> &cmd) {
        if (cmd.empty()) {
            for (auto x: _cmds) {
                cout << x.second.helpDoc << endl;
            }
        } else {
            auto it = _cmds.find(cmd[0]);
            if (it == _cmds.end()) {
                cout << "help: no help topics match " << cmd[0] << " ." << endl;
                return false;
            } else {
                cout << it->second.helpDoc << endl;
            }
        }
        return true;
    };

    string _parseArgum(vector<string> &param) {
        for (auto p: param) {
            if (p[0] != '-') return p;
        }
        return "";
    }

    string _parseParam(vector<string> &param) {
        string result;
        for (auto p: param) {
            if (p[0] == '-') result += p.substr(1, p.length() - 1);
        }
        return result;
    }

    string _inputPassword(int size = _PASSWORD_LENGTH) {
        int ch, p = 0;
        char password[_PASSWORD_LENGTH + 1] = {0};
        while (true) {
            password[p] = static_cast<char>(_getch());
            if (password[p] == '\b') {
                if (p > 0) {
                    p = std::max(0, p - 1);
                    cout << "\b \b";
                }
            } else {
                cout << "*";
                p ++;
            }
            if (p >= size) {
                while ((ch = _getch()) != '\r' && ch != '\b');
                if (ch == '\b') {
                    p = size - 1;
                    cout << "\b \b";
                } else {
                    cout << endl;
                    break;
                }
            }
        }
        return string(password);
    }
};