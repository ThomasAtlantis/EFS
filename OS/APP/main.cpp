//
// Created by Bessie on 2019/6/25.
#include "../../utilities.h"
#include "CLIController.h"
#include <cstdio>
#include <afxres.h>
#include <locale.h>

int main(int argc, char * argv[]){
    CLIController cliController;
    while(1){
        cout << "root@USER root/" << endl;
        cout << "$  ";
        cliController.ReadCommand();
        cout << endl;
    }
//    FileTool fileTool;
//    vector<string> vector1 = fileTool.dir("../VirtualMachine");
//    for(vector<string>::iterator iter1=vector1.begin(); iter1!=vector1.end(); ++iter1 ){
//        for( size_t sty=0;sty!=(*iter1).size(); ++sty){
//            cout<< (*iter1)[sty] ; //*iter1才是string数据
//        }
//        cout << endl;
//    }
//    string files, majorDisk;
//    for (const auto &fileName: vector1) {
//        if (FileTool().ext1(fileName) == "OS") {
//            majorDisk = fileName;
//            break;
//        }
//    }
//    cout << "ok!" << majorDisk << endl;
    return 0;
}