//
// Created by Bessie on 2019/6/25.
//

#ifndef EFS_CLICONTROLLER_H
#define EFS_CLICONTROLLER_H

#include <afxres.h>
#include <string>
#include <iomanip>
#include "../../utilities.h"
using namespace std;
class CLIController {

private:
    string fileName;

    FileTool fileTool;

public:
    CLIController(){
        cout << "--------Welcome!--------" << endl;
    }
    ~CLIController(){}

    bool ReadCommand(){
        string command;
        cin >> command;
       vector<string> vector2 = CommandResolution(command, " ");//命令行解析
        string pCommand = vector2.at(0);
        if(pCommand == "ls"){//列出目录
            FileTool fileTool;
            vector<string> vector1 = fileTool.dir("../VirtualMachine");
            int flag = 0;
            for(vector<string>::iterator iter1 = vector1.begin(); iter1!=vector1.end(); ++iter1 ){
                string str = "";
                for( size_t sty=0;sty!=(*iter1).size(); ++sty){

                    str += (*iter1)[sty]; //*iter1才是string数据
                }
                flag++;
                cout << setw(15) << setiosflags(ios::left) << str + "/";
                if(flag%3 == 0) cout << endl;
            }
            cout << endl;
        }else if(pCommand == "ll"){

        }else if(pCommand == "cd"){

        }else if(pCommand == "openr"){

        }else if(pCommand == "openw"){

        }else if(pCommand == "mkdir"){

        }else if(pCommand == "vim"){

        }else if(pCommand == "rm"){

        }else if(pCommand == "cd"){

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

    vector<string> CommandResolution(string command, string flag){
        string str = command;
        vector<string> pCommand;
        size_t index = 0;
        while(index < command.rfind(flag)){
            index = str.find(flag);
            pCommand.push_back(str.substr(0, index));
            str = str.substr(index+1, command.length()-index);
        }
        return pCommand;
    }

};


#endif //EFS_CLICONTROLLER_H
