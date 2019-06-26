//
// Created by MAC on 2019/6/19.
//

#ifndef EFS_UTILITIES_H
#define EFS_UTILITIES_H

#include <io.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::cin;

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
        size_t index = fileName.rfind('.');
        return fileName.substr(index + 1, fileName.length() - index - 1);
    }
    string name(string fileName) {
        return fileName.substr(0, fileName.rfind('.'));
    }
};

class BufferTool {
public:
    void copy(char * dst, const char * src, size_t length) {
        for (size_t i = 0; i < length; ++ i) dst[i] = src[i];
    }
};

#endif //EFS_UTILITIES_H
