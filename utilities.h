//
// Created by MAC on 2019/6/19.
//

#ifndef EFS_UTILITIES_H
#define EFS_UTILITIES_H

#include <io.h>
#include <iostream>
#include <vector>
#include <string>

using std::string;
using std::vector;
using std::cout;
using std::endl;

#define all(x) (x).begin(),(x).end()

class FileTool {
public:
    vector<string> dir(string path = ".", bool recursive = false) {
        vector<string> files;
        intptr_t hFile = 0;
        struct _finddata_t fileInfo{}; //用来存储文件信息的结构体
        string p;
        if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileInfo)) != -1) {
            do {
                if (strcmp(fileInfo.name, ".") == 0 || strcmp(fileInfo.name, "..") == 0) continue;
                if ((fileInfo.attrib &  _A_SUBDIR) && recursive) { //进入文件夹查找
                    files.push_back(p.assign(path).append("\\").append(fileInfo.name));
                    vector<string> tmp = dir(p.assign(path).append("\\").append(fileInfo.name), true);
                    files.insert(files.end(), all(tmp));
                } else files.push_back(p.assign(fileInfo.name));
            } while(_findnext(hFile, &fileInfo) == 0);
            _findclose(hFile); //结束查找
        }
        return files;
    }
    string ext(string fileName) {
        for (size_t i = fileName.length() - 1; i >= 0; -- i)
            if (fileName[i] == '.')
                return fileName.substr(i + 1, fileName.length() - i - 1);
    }
};

#endif //EFS_UTILITIES_H
