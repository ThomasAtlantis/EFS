//
// Created by Bessie on 2019/6/25.
//

#pragma once

#include <utility>
#include "../../utilities.h"
#include "../VFS/vfs.h"
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
    INode * _curINode;
    Screen _screen;
    string _curPath = "~";
    string _symbol = "#";
    string _machineName = "@VirtualMachine";
    map<string, Command> _cmds;
    string _cmdLine;

public:
    App(VFSController &vfs): _vfs(vfs) {
        bind("help", help, "help: help <command>\n    Display information about builtin commands.");
        bind("touch", touch, "touch: touch <filename>\n    Create a new empty file.");
    }

    bool run() {
//        freopen("data.in", "r", stdin);
//        freopen("data.out", "w", stdout);
        login();
        do {
            commandTip();
        } while (readCommand());
        return true;
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
        cin >> userName;
//        if (!_vfs.existUser(userName)) return false;
        cout << userName << _machineName << "'s password: ";
        cin >> password;
//        if (!_vfs.match(userName, password)) return false;
        cout << "Last login: " << endl;
        cout << "Welcome to VirtualMachine X!" << endl;
        return true;
    }

    bool touch(vector<string> &param) {
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

    string _inputPassword(int size = _PASSWORD_LENGTH) {
        int ch, p = 0;
        char password[_PASSWORD_LENGTH + 1] = {0};
        while (true) {
            password[p ++] = static_cast<char>(_getch());
            if (password[p - 1] == '\b') {
                if (p > 1) {
                    p = std::max(0, p - 2);
                    cout << "\b \b";
                }
            } else cout << "*";
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
