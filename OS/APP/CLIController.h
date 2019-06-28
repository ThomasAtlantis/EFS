//
// Created by Bessie on 2019/6/25.
//

#ifndef EFS_CLICONTROLLER_H
#define EFS_CLICONTROLLER_H

#include <afxres.h>
#include <string>
#include <iomanip>
#include <stdlib.h>
#include <conio.h>
#include <iostream>
#include <fstream>
#include "../../utilities.h"
#include "light.h"
#include "../FSC/fsc.h"

using std::ifstream;
using std::ofstream;

#define DEBUG 0

using namespace std;
class CLIController {

private:
    string fileName;

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
    CLIController(){
        initUser();
        string str = "--------Welcome!--------";
        light.cprintf((char*)str.data(), 10);
        cout << endl;
        //loginSystem();
        //InitUser();
    }
    ~CLIController(){}

    vector<string> getRootDir(){
        vector<string> vector1;
        vector1.push_back(rootDir);
        vector1.push_back(startSymble);
        return vector1;
    }

    bool readCommand(){
        char command[40];
        #if DEBUG
        strcpy(command, "useradd -c<password> username");
        #else
        fflush(stdin);
        gets(command);
        #endif
        vector<string> vector2 = stringTool1.split(command, " ");//命令行解析
        string pCommand = vector2.at(0);
        int preference = vector2.size()-1;
        if(pCommand == "ls"){//列出目录
            if(preference == 0){ //当前目录非隐藏目录
                vector<string> vector1 = fileTool.dir(testDirectory);
                int flag = 0;
                for(vector<string>::iterator iter1 = vector1.begin(); iter1!=vector1.end(); ++iter1 ){
                    string str = "";
                    for( size_t sty=0;sty!=(*iter1).size(); ++sty){
                        str += (*iter1)[sty]; //*iter1才是string数据
                    }
                    flag++;
                    cout << std::setw(20) << std::setiosflags(std::ios::left) << str;
                    if(flag%3 == 0) cout << endl;
                }
                cout << endl;
            }else if(preference == 1){ //显示当前目录所有文件（包括隐藏的）
                if(vector2[1] == "-a"){
                    cout << "Show all files and directories" << endl;
                }
            }else if(preference == 2){ //查看文件的时间属性
                if(vector2[1] == "-l"){
                    cout << "-rw-r--r-- 1 hdd hdd 55 2011-08-22 19:53 " << vector2.at(2) << endl;
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
        }else if(pCommand == "touch"){ //创建文件
            vector<string> vector1 = fileTool.dir(testDirectory);
            int flag;
            for(int i=0; i<vector1.size(); i++){
                if( ! vector2.at(1).compare(vector1.at(i))){ //修改文件的时间属性
                    cout << vector2.at(1) << " has been set to the current system time" << endl;
                    break;
                }
                flag = i + 1;
            }
            if(flag == vector1.size()){ //创建新文件
                cout << vector2.at(1) << " create successfully" << endl;
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

    string inputPassword(int size = _PASSWORD_LENGTH) {
        int ch, p = 0;
        char password[_PASSWORD_LENGTH + 1] = {0};
        while (true) {
            password[p ++] = static_cast<char>(_getch());
            if (password[p - 1] == '\b') {
                if (p > 1) {
                    p = max(0, p - 2);
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

};



#endif //EFS_CLICONTROLLER_H
