//
// Created by Bessie on 2019/6/25.
//

#pragma once

#include "../../utilities.h"
#include "../VFS/vfs.h"
#include "../FSC/fsc.h"
#include "screen.h"

using std::ifstream;
using std::ofstream;

#define DEBUG 1
#define WIDTH 108
#define HEIGHT 50

class App {

private:
    typedef FSController::INode INode;
    typedef struct {
        string helpDoc;
        bool (App::*func)(vector<string>&);
    } Command;
    VFSController &_vfs; // 虚拟文件系统
    Screen _screen;
    string _curPath = "/usr";
    string _symbol = "#";
    string _machineName = "@VirtualMachine";
    map<string, Command> _cmds;
    string _cmdLine;
    vector<string> history;
    char _buffer[WIDTH * HEIGHT];
    HANDLE hout;

public:
    App(VFSController &vfs): _vfs(vfs) {
        bind("help", help, "help: help <command>\n    Display information about builtin commands.");
        bind("touch", touch, "touch: touch <fileName>\n    Create a new empty file.");
        bind("ld", listDir, "ld: ld [-s|-l|-a] [dirName]\n    List files in a directory.");
        bind("clear", clear, "clear: clear\n    Clear the screen.");
        bind("rm", remove, "rm: rm [-r][-f] <fileName>\n    Remove a file.");
        bind("mkdir", makeDir, "mkdir: mkdir <dirName>\n    Create a new empty directory.");
        bind("cd", changeDir, "cd: cd <DirName>\n    Change the current working directory.");
        bind("pwd", pwd, "pwd: pwd\n    Print the name of the current working directory.");
        bind("vim", edit, "vim: vim <fileName>\n    Edit text file.");
        bind("cat", print, "cat: cat <fileName>\n    print the content of text file.");
        history = {""};
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
        cout << " \b";
        _cmdLine = "";
        union {
            char ch[2];
            int key;
        } symbol {};
        int historyIndex = 0;
        int cursorIndex = 0;
        symbol.key = getch();
        while (symbol.ch[0] != '\r') {
            if (symbol.ch[0] == -32) {
                symbol.key = getch();
                switch (symbol.ch[0]) {
                    case 72: // 上
                        if (historyIndex + 1 < history.size()) historyIndex ++;
                        for (int i = 0; i < _cmdLine.length(); ++ i) cout << "\b \b";
                        _cmdLine = history[historyIndex];
                        cursorIndex = static_cast<int>(_cmdLine.length());
                        cout << _cmdLine;
                        break;
                    case 80: // 下
                        if (historyIndex > 0) historyIndex --;
                        for (int i = 0; i < _cmdLine.length(); ++ i) cout << "\b \b";
                        _cmdLine = history[historyIndex];
                        cursorIndex = static_cast<int>(_cmdLine.length());
                        cout << _cmdLine;
                        break;
                    case 75: // 左
                        if (cursorIndex > 0) {
                            cout << "\b" << _cmdLine[-- cursorIndex] << "\b";
                        }
                        break;
                    case 77: // 右
                        if (cursorIndex < _cmdLine.length()) {
                            cout << _cmdLine[cursorIndex ++];
                        }
                        break;
                }
            } else if (symbol.ch[0] == '\b') {
                if (cursorIndex > 0) {
                    for (int i = 0; i < cursorIndex; ++ i) cout << "\b \b";
                    string newCmdLine;
                    if (cursorIndex != 1) newCmdLine += _cmdLine.substr(0, cursorIndex - 1);
                    size_t cnt = _cmdLine.length() - cursorIndex;
                    if (cursorIndex != _cmdLine.length())
                        newCmdLine.append(_cmdLine.substr(cursorIndex, _cmdLine.length() - cursorIndex));
                    _cmdLine = newCmdLine;
                    cout << _cmdLine << " \b";
                    for (int i = 0; i < cnt; ++ i) cout << "\b";
                    cursorIndex --;
                }
            } else if (symbol.ch[0] == '\t') {
                vector<string> cmdList = stringTool().split(_cmdLine, " ");
                vector<string> recommends {};
                if (cmdList.empty() || _cmdLine.back() == ' ') {
                    symbol.key = getch();
                    continue;
                }
                if (cmdList.size() == 1) {
                    for (const auto &x: _cmds) {
                        if (x.first.find(cmdList[0]) == 0) {
                            recommends.push_back(x.first);
                        }
                    }
                } else if (cmdList.size() > 1) {
                    string tmp = cmdList.back();
                    string fileName, dirName;
                    size_t index = tmp.rfind('/');
                    if (index == string::npos) {
                        fileName = tmp;
                        dirName = ".";
                    } else {
                        dirName = tmp.substr(0, index);
                        fileName = tmp.substr(index + 1, tmp.length() - index - 1);
                    }
                    int error = 0;
                    for (const auto &x: _vfs.listDir(error, dirName)) {
                        if (string(x->name).find(fileName) == 0) {
                            recommends.emplace_back(x->name);
                        }
                    }
                }
                int index = 0;
                if (recommends.empty()) {
                    symbol.key = getch();
                    continue;
                }
                string tmp = cmdList.back();
                size_t x = tmp.rfind('/');
                size_t length = tmp.length();
                if (x != string::npos) length -= (x + 1);
                size_t where;
                while (symbol.ch[0] == '\t') {
                    for (size_t i = 0; i < length; ++i) cout << "\b \b";
                    cout << recommends[index];
                    length = recommends[index].length();
                    if (x != string::npos) where = _cmdLine.rfind('/');
                    else where = _cmdLine.rfind(' ');
                    _cmdLine = _cmdLine.substr(0, where + 1).append(recommends[index]);
                    index = (index + 1) % static_cast<int>(recommends.size());
                    symbol.key = getch();
                }
                continue;
            } else if (symbol.ch[0] <= 126 && symbol.ch[0] >= 32) {
                symbol.ch[1] = '\0';
                for (int i = 0; i < cursorIndex; ++ i) cout << "\b \b";
                string newCmdLine;
                if (cursorIndex != 0) newCmdLine += _cmdLine.substr(0, cursorIndex);
                newCmdLine.append(symbol.ch);
                size_t cnt = _cmdLine.length() - cursorIndex;
                if (cursorIndex != _cmdLine.length())
                    newCmdLine.append(_cmdLine.substr(cursorIndex, _cmdLine.length() - cursorIndex));
                _cmdLine = newCmdLine;
                cout << _cmdLine;
                for (int i = 0; i < cnt; ++ i) cout << "\b";
                cursorIndex ++;
            }
            symbol.key = getch();
        }
        cout << endl;
        fflush(stdin);
//        getline(cin, _cmdLine);
        if (_cmdLine.empty()) return true;
        history.insert(history.begin() + 1, _cmdLine);
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

    bool edit(vector<string> &param) {
        if (param.empty()) {
            cout << missParam("vim", "<fileName>") << endl;
            return false;
        }
        int error = 0;
        string fileName = param[0];
        strcpy(_buffer, "hello world!");
        // open
        // read to buffer
        // show buffer
        system("cls");
        cout << endl;
        cout << _buffer;
        bool overFlag = false;
        hout = GetStdHandle(STD_OUTPUT_HANDLE);
        MSG msg = {0};
        HWND hConsole = GetActiveWindow();
        RegisterHotKey(hConsole, 0, 0x4000| MOD_CONTROL, VK_RETURN);
        RegisterHotKey(hConsole, 1, 0x4000, 'I');
        while (GetMessage(&msg, NULL, 0, 0) != 0) {
            if (msg.message == WM_HOTKEY) {
                if (msg.wParam == 0) {
                    _gotoXY(hout, 0, 0);
                    cout << ":          ";
                    _gotoXY(hout, 1, 0);
                    bool loop = true;
                    while (loop) {
                        string cmd;
                        char ch[2] = {0};
                        ch[0] = static_cast<char>(getch());
                        while (ch[0] != '\r') {
                            cmd += ch;
                            if (ch[0] == 27) {
                                _gotoXY(hout, 0, 0);
                                cout << "           ";
                                _gotoXY(hout, 0, 0);
                                loop = false;
                                break;
                            }
                            cout << ch;
                            ch[0] = static_cast<char>(getch());
                        }
                        if (!loop) break;
                        if (cmd == "q") {
                            overFlag = true;
                            break;
                        }
                        _gotoXY(hout, 1, 0);
                        cout << "          ";
                        _gotoXY(hout, 1, 0);
                    }
                } else if (msg.wParam == 1) {
                    _gotoXY(hout, 0, 0);
                    cout << "< INSERT >";
                    int x = 0, y = 1;
                    _gotoXY(hout, 0, 1);
                    union {
                        char ch[2];
                        int key;
                    } symbol {};
                    while (symbol.ch[0] != 27) {
                        symbol.key = getch();
                        if (symbol.ch[0] == -32) {
                            symbol.key = getch();
                            switch (symbol.ch[0]) {
                                case 75:
                                    x = std::max(x - 1, 0);
                                    _gotoXY(hout, x, y); // 左移
                                    break;
                                case 72:
                                    y = std::max(y - 1, 1);
                                    _gotoXY(hout, x, y); // 上移
                                    break;
                                case 80:
                                    y ++;
                                    _gotoXY(hout, x, y); // 下移
                                    break;
                                case 77:
                                    x = std::min(x + 1, WIDTH);
                                    _gotoXY(hout, x, y); // 右移
                                    break;
                            }
                        } else if (symbol.ch[0] == '\b') {
                            if (x == WIDTH) {
                                cout << " \b\b \b";
                                x = x - 1;
                            } else if (x > 0) {
                                cout << "\b \b";
                                x = x - 1;
                            }
                        } else if (symbol.ch[0] >= 32 && symbol.ch[0] <= 126) {
                            if (x <= WIDTH) {
                                cout << symbol.ch ;
                                if (x < WIDTH) {
                                    x = x + 1;
                                }
                                _gotoXY(hout, x, y);
                            }
                        }
                    }
                    _gotoXY(hout, 0, 0);
                    cout << "           ";
                    _gotoXY(hout, 0, 0);
                    fflush(stdin);
                }
            }
            if (overFlag) break;
        }
        UnregisterHotKey(hConsole, 0);
        UnregisterHotKey(hConsole, 1);
        system("cls");
        // save buffer
        return true;
    }

    bool print(vector<string> &param) {
        return true;
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
            cout << "mkdir: cannot create '" << param[0] << "': ";
            if (error == -3) cout << "File already exists";
            else if (error == -5) cout << "FileName too long";
            else if (error == -6) cout << "FileName illegal";
            cout << endl;
            return false;
        }
        return true;
    }

    bool changeDir(vector<string> &param) {
        if (param.empty()) {
            cout << missParam("cd", "<dirName>") << endl;
            return false;
        }
        int error = 0;
        string dirName = param[0];
        if (!_vfs.changeDir(error, dirName)) {
            cout << "cd: cannot access '" << param[0] << "': ";
            if (error == -3) cout << "No such directory";
            else if (error == -4) cout << "Permission denied";
            else if (error == -5) cout << "DirName too long";
            else if (error == -6) cout << "DirName illegal";
            cout << endl;
            return false;
        }
        _curPath = _vfs.workingDir(_vfs.curPart, _vfs.curINode);
        return true;
    }

    bool pwd(vector<string> &param) {
        cout << _vfs.workingDir(_vfs.curPart, _vfs.curINode) << endl;
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
        int partNum; string fileNameCopy = fileName;
        INode * iNode = _vfs.parsePath(partNum, fileNameCopy);
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
        int error = 0;
        bool result = _vfs.removeFile(error, partNum, iNode);
        if (error == -4) {
            cout << "rm: cannot remove '" << fileName << "': Permission denied" << endl;
            return false;
        }
        return result;
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
    bool listDir(vector<string> &vec) {
        string dirName = _parseArgum(vec);
        string params = _parseParam(vec);
        bool longFlag = false;
        bool allFlag = false;
        for (auto p: params) {
            if (p == 'l') longFlag = true;
            if (p == 's') longFlag = false;
            if (p == 'a') allFlag = true;
        }
        if (dirName.empty()) dirName = ".";
        int error = 0;
        vector<INode *> list = _vfs.listDir(error, dirName, allFlag);
        if (error == -3) {
            cout << "ld: cannot access '" << dirName << "': No such file or directory" << endl;
            return false;
        }
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

    void _gotoXY(HANDLE hout, int X, int Y) {
        COORD coord;
        coord.X = static_cast<SHORT>(X);
        coord.Y = static_cast<SHORT>(Y);
        SetConsoleCursorPosition(hout, coord);
    }

    /* 在指定位置处打印一个指定的字符 */
    void _showChar(HANDLE hout, int X, int Y, char ch) {
        _gotoXY(hout, X, Y);
        putchar(ch);
    }

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
            password[p] = static_cast<char>(getch());
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
                while ((ch = getch()) != '\r' && ch != '\b');
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
