//
// Created by Bessie on 2019/6/25.
//

#pragma once

#include "../../utilities.h"
#include "../VFS/vfs.h"
#include "light.h"

using std::ifstream;
using std::ofstream;

#define DEBUG 1

class VFSController;

class App {

private:
    string fileName;

    VFSController * _vfs;
    FSController::INode* curINode;


    //fsc::INode =
    //fsc.newINode();

    Light light;

    string rootDir = "root@USER root";
    string startSymble = "$";

    map<string, string> users;

    FileTool fileTool;
    stringTool stringTool1;

    string testDirectory = "../VirtualMachine";

public:
    App() {
        initUser();
        string str = "--------Welcome!--------";
        light.cprintf((char*)str.data(), 10);
        cout << endl;
        //loginSystem();
        //InitUser();
    }
    ~App(){}

    vector<string> getRootDir(){
        vector<string> vector1;
        vector1.push_back(rootDir);
        vector1.push_back(startSymble);
        return vector1;
    }

    bool readCommand(){
        char command[40];
        strcpy(command, "touch hdjhj");
//        fflush(stdin);
//        gets(command);
        vector<string> vector2 = stringTool1.split(command, " ");//命令行解析
        string pCommand = vector2.at(0);
        int preference = vector2.size()-1;
        if(pCommand == "touch"){ //创建文件

        }
        if(pCommand == "ls"){//列出目录
            vector<FSController::INode*> vector3;
            //= _fsc->listDir(*curINode);
            if(preference == 0){ //当前目录非隐藏目录
                int flag = 0;
                for (auto iNode: vector3) {
                    string str = iNode->name;
                    cout << std::setw(20) << std::setiosflags(std::ios::left) << str;
                    flag++;
                }
                if(flag%3 == 0) cout << endl;
                cout << endl;
            }else if(preference == 1){ //显示当前目录所有文件（包括隐藏的）
                if(vector2[1] == "-a"){
                    cout << "Show all files and directories" << endl;
                }
            }else if(preference == 2){ //查看文件的时间属性
                if(vector2[1] == "-l"){
                    for (auto iNode: vector3) {
                        cout << std::tolower(iNode->type);
                        for (char i : iNode->mode) {
                            cout << (i & mode::read) ? "r": "-";
                            cout << (i & mode::write) ? "w": "-";
                            cout << (i & mode::exec) ? "x": "-";
                        }
                        cout << " ";
                        if (strlen(iNode->owner) > 6) cout << string(iNode->owner).substr(0, 6);
                        else cout << std::setw(6) << iNode->owner;
                        cout << " ";
                        std::stringstream ss;
                        ss << std::put_time(std::localtime(&iNode->ctime), "%Y-%m-%d %H:%M ") << endl;
                        cout << iNode->name << endl;
                    }
                }
            }
        }else if(pCommand == "ll"){

        }else if(pCommand == "cd"){
            if(preference == 1){
                //cout << vector2[1] << endl;
                if(vector2[1] == ".."){
                    vector<string> vector1 = stringTool1.split(rootDir, "/");
                    if(vector1.size() == 1){
                        cout << "arrive at root dirctory already" << endl;
                    } else{
                        rootDir = vector1.at(0);
                        for(int i=1; i<vector1.size()-1; i++){
                            rootDir += "/" + vector1.at(i);
                        }
                    }
                }else{
                    rootDir += "/" + vector2[1];
                }
            }
        }else if(pCommand == ""){

        }else if(pCommand == "read"){
            cout << "TODO:读文件 " << endl;
        }else if(pCommand == "write"){
            cout << "TODO:写文件" << endl;
        }else if(pCommand == "vi"){

        }else if(pCommand == "rm"){

        }else if(pCommand == "chmodv"){

        }else if(pCommand == "mv"){

        }else if(pCommand == "userdel"){  //删除用户
            if(preference == 1){
                map<string, string>::iterator it = users.find(vector2.at(1));
                if(it == users.end()){
                    cout << "no user" << endl;
                }else{
                    users.erase(vector2.at(1));
                    cout << "delete successfully" << endl;
                }
            }else{
                cout << "-f：强制删除用户，即使用户当前已登录；" << endl;
                cout << "-r：删除用户的同时，删除与用户相关的所有文件。" << endl;
            }
        }else if(pCommand == "useradd"){    //添加用户
            string str1 = vector2.at(1);
            if( ! str1.substr(1, 1).compare("c") ){//useradd -c<password> username
                string str = vector2.at(1);
                int index = str.find('<');
                int index1 = str.find('>');
                users.insert(std::pair<string, string>(vector2.at(2), str.substr(index+1, index1-index-1)));
                cout << "add successfully" << endl;
            }else{
                cout << "TODO" << endl;
            }
        }else if(pCommand == "passwd"){

        }else if(pCommand == "su"){

        }else if(pCommand == "format"){

        }else if(pCommand == "echo"){

        }else if(pCommand == "cat"){

        }else if(pCommand == "clear"){

        }else if(pCommand == "exit"){
            exit(0);
        }else if(pCommand == "help"){

        }else if(pCommand == "clear"){
            system("cls");
        }else{
            cout << "not found command" << endl;
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

    bool openCommandFile(string commond){
        PROCESS_INFORMATION ProcessInfo;
        STARTUPINFO StartupInfo; //This is an [in] parameter。创建新进程时，它将使用该结构的有关成员
        ZeroMemory(&StartupInfo, sizeof(StartupInfo));//将该结构中的所有成员初始化为零
        StartupInfo.cb = sizeof StartupInfo ; //Only compulsory field。用作版本控制手段
        vector<string> vector1 = fileTool.dir(testDirectory);
        for (const auto &files: vector1) {
            if (FileTool().ext1(files) == commond) {
                fileName = files;
                break;
            }
        }
        string str = testDirectory + fileName;
        if(CreateProcess( str.c_str(), NULL, NULL,NULL,FALSE,0,NULL, NULL,&StartupInfo,&ProcessInfo))
        {
            WaitForSingleObject(ProcessInfo.hProcess,INFINITE);
            CloseHandle(ProcessInfo.hThread);
            CloseHandle(ProcessInfo.hProcess);
        }
//        else
//            MessageBox(NULL,"The process could not be started",NULL,NULL);
        return true;
    };

    void loginSystem(){
        map<string, string> ::iterator it;
        while(1){
            string userName;
            cout << "login: ";
            #if DEBUG
            userName = "USER";
            #else
            cin >> userName;
            #endif
            it = users.find(userName);
            if(it == users.end()){
                cout << "no this user" << endl;
                continue;
            }
            break;
        }
        while(1){
            string userPassword;
            cout << "Password: ";
            #if DEBUG
            userPassword = "123456";
            #else
            userPassword = getPasswordWithoutPlainData();
            #endif
            if(it->second != userPassword){
                cout << "your password is error, please input again" << endl;
                continue;
            }
            system("cls");
            break;
        }
    }

    void initUser(){
        users.insert(std::pair<string, string>("USER", "123456"));
        //cout << "init successfully";
    }

    string getPasswordWithoutPlainData()
    {
        string ret;
        char ch;
        ch = _getch();
        while (ch != '\n' && ch != '\r')
        {
            ret += ch;
            //cout << "debug:" << ret << endl;
            ch = _getch();
        }
        return ret;
    }

    void writeFile(string filename){
        ofstream outfile(filename, std::ios::trunc);
        char context[256];
        fflush(stdin);
        gets(context);
        //将数据输出至out.txt文件中
        for (int i = 0; i < 100; i++)
        {
            outfile << context[i] << endl;
        }
        outfile.close();
    };

    void readFile(string fileName){
        ifstream myfile(fileName);
        char buffer[256];
        if (!myfile.is_open())
        {
            cout << "can not open this file" << endl;
            return ;
        }
        while (! myfile.eof() ) {
            myfile.getline(buffer,100);
            cout << buffer << endl;
        }
        myfile.close();
    };

    bool run() {
        freopen("data.in", "r", stdin);
        freopen("data.out", "w", stdout);
        loginSystem();
        while (true) {
            cout << "root@USER root/ " << endl;
            cout << "$ ";
            if (!readCommand()) break;
            cout << endl;
        }
        return true;
    }

    void over() {
        cout << "done!" << endl << endl;
    }

    void help(){
        cout << "ls [-a] [-l filename]" << endl; //列出目录
        cout << "ll" << endl; //列出当前详细目录
        cout << "cd [..] [filename]" <<endl; //目录切换
        cout << "touch <filename>" << endl; //创建文件
        cout << "read <filename>" << endl; //读文件
        cout << "write <filename>" << endl; //写文件
        cout << "vi <filename> " << endl; //打开文件
        cout << "rm  [-r] <filename>" << endl; //删除文件
        cout << "mv <filename> <filename>" << endl; //移动文件
        cout << "userdel <username>" << endl; //删除用户
        cout << "useradd <username>" << endl; //添加用户
        cout << "passwd [-d] [-S] <username>" << endl; //修改密码
        cout << "su <username>" << endl; //变更为其他使用者的身份
        cout << "fomat" << endl; //格式化
        cout << "" << endl;
        cout << "" << endl;
        cout << "" << endl;
    };

    // For OS installation

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

    void showPartSize(const bid_t &sysSize, const bid_t &_sliceSize) {
        for (int i = 0; i < std::to_string(_sliceSize).length()
            - std::to_string(sysSize).length() + 4; ++ i) cout << " ";
        cout << "[";
        for (int i = 0; i < sysSize * 20 / _sliceSize; ++ i)
            cout << "-";
        cout << "]" << endl;
    }

    string initPassword() {
        cout << "Initialize user system ... " << endl;
        cout << "Initial super user: " << _SUPERADMIN_NAME << endl;
        cout << ">> Initial password: ";
        #if DEBUG
        string password = "123456";
        #else
        int failCount = 0;
        string password = _inputPassword();
        while (true) {
            cout << ">> Repeat password: ";
            string repeat = _inputPassword();
            if (repeat == password) break;
            else {
                cout << "Wrong Repetition!" << endl;
                if (failCount ++ == 3) {
                    cout << "rollback ... " << endl;
                    cout << ">> Initial password: ";
                    password = _inputPassword();
                }
            }
        }
        #endif
        return password;
    }

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
};
