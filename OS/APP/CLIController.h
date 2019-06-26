//
// Created by Bessie on 2019/6/25.
//

#ifndef EFS_CLICONTROLLER_H
#define EFS_CLICONTROLLER_H

#include <afxres.h>
#include <string>
#include <iomanip>
#include <stdlib.h>
#include "../../utilities.h"

using namespace std;
class CLIController {

private:
    string fileName;

    string rootDir = "root@USER root";
    string startSymble = "$";

    FileTool fileTool;
    stringTool stringTool1;

public:
    CLIController(){
        cout << "--------Welcome!--------" << endl;
    }
    ~CLIController(){}

    vector<string> getRootDir(){
        vector<string> vector1;
        vector1.push_back(rootDir);
        vector1.push_back(startSymble);
        return vector1;
    }

    bool ReadCommand(){
        char command[20];
        gets(command);
        vector<string> vector2 = stringTool1.split(command, " ");//命令行解析
        string pCommand = vector2.at(0);
        int preference = vector2.size()-1;
        if(pCommand == "ls"){//列出目录
            if(preference == 0){
                vector<string> vector1 = fileTool.dir("../VirtualMachine");
                int flag = 0;
                for(vector<string>::iterator iter1 = vector1.begin(); iter1!=vector1.end(); ++iter1 ){
                    string str = "";
                    for( size_t sty=0;sty!=(*iter1).size(); ++sty){
                        str += (*iter1)[sty]; //*iter1才是string数据
                    }
                    flag++;
                    cout << setw(20) << setiosflags(ios::left) << str;
                    if(flag%3 == 0) cout << endl;
                }
                cout << endl;
            }else if(preference == 1){
                if(vector2[1] == "-a"){
                    cout << "Show all files and directories" << endl;
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
                    system("cls");
                    rootDir += "/" + vector2[1];
                }
            }
        }else if(pCommand == "openr"){

        }else if(pCommand == "openw"){

        }else if(pCommand == "mkdir"){

        }else if(pCommand == "vim"){

        }else if(pCommand == "rm"){

        }else if(pCommand == "chmodv"){

        }else if(pCommand == "mv"){

        }else if(pCommand == "userdel"){

        }else if(pCommand == "passwd"){

        }else if(pCommand == "su"){

        }else if(pCommand == "format"){

        }else if(pCommand == "echo"){

        }else if(pCommand == "cat"){

        }else if(pCommand == "clear"){

        }else if(pCommand == "exit"){
            exit(0);
        }else{
            cout << "command not found" << endl;
        }
    }

    bool OpenCommandFile(string commond){
        PROCESS_INFORMATION ProcessInfo;
        STARTUPINFO StartupInfo; //This is an [in] parameter。创建新进程时，它将使用该结构的有关成员
        ZeroMemory(&StartupInfo, sizeof(StartupInfo));//将该结构中的所有成员初始化为零
        StartupInfo.cb = sizeof StartupInfo ; //Only compulsory field。用作版本控制手段
        vector<string> vector1 = fileTool.dir("../VirtualMachine");
        for (const auto &files: vector1) {
            if (FileTool().ext1(files) == commond) {
                fileName = files;
                break;
            }
        }
        string str = "../VirtualMachine/" + fileName;
        if(CreateProcess( str.c_str(), NULL, NULL,NULL,FALSE,0,NULL, NULL,&StartupInfo,&ProcessInfo))
        {
            WaitForSingleObject(ProcessInfo.hProcess,INFINITE);
            CloseHandle(ProcessInfo.hThread);
            CloseHandle(ProcessInfo.hProcess);
        }
        else
            MessageBox(NULL,"The process could not be started",NULL,NULL);
    };

};


#endif //EFS_CLICONTROLLER_H
