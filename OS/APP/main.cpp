//
// Created by Bessie on 2019/6/25.
#include "../../utilities.h"
#include "app.h"
#include <cstdio>
#include "screen.h"


int main(int argc, char * argv[]){

    Light light;
    CLIController cliController;
    while(1){
        vector<string> vector1 = cliController.getRootDir();
        light.cprintf((char*)vector1.at(0).data(), 10);
        cout << endl;
        light.cprintf((char*)vector1.at(1).data(), 10);
        cliController.readCommand();
        cout << endl;
    }
    return 0;
}