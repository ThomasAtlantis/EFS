//
// Created by MAC on 2019/6/19.
//

#ifndef EFS_UTILITIES_H
#define EFS_UTILITIES_H

#include <io.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <map>
#include <cctype>
#include <afxres.h>
#include <cstdlib>
#include <conio.h>
#include <fstream>
#include <sstream>

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::cin;
using std::map;

#define all(x) (x).begin(),(x).end()

class FileTool {
public:
    vector<string> dir(string path = ".", bool recursive = false) { //找到对应文件夹下所有文件目录
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
    string ext1(string fileName) { //找到对应文件名的文件全称
        size_t index = fileName.rfind('.');
        return fileName.substr(0, index);
    }

};

class BufferTool {
public:
    void copy(char * dst, const char * src, size_t length) {
        for (size_t i = 0; i < length; ++ i) dst[i] = src[i];
    }
};

class stringTool {
public:
    vector<string> split(string str, string sep) {//将str用sep分割成多个子串
        vector<string> result;
        size_t index = str.find(sep);
        while (index != string::npos)
        {
            result.push_back(str.substr(0, index));
            str = str.substr(index + sep.length(),str.length() - index - sep.length());
            while (str.find(sep) == 0) {
                str = str.substr(sep.length(), str.length() - sep.length());
            }
            index = str.find(sep);
        }
        if (str.length()) result.push_back(str);
        return result;
    }
    vector<string> split(char * data, size_t size, char sep) {
        vector<string> result;
        size_t begin = 0;
        for (size_t i = 0; i < size; ++ i) {
            if (data[i] == sep) {
                data[i] = '\0';
                if (i > begin) result.emplace_back(data + begin);
                data[i] = sep;
                begin = i + 1;
            }
        }
        return result;
    }
    char * strip(char * data, size_t size, char trim) {
        size_t lIndex = 0, rIndex = size - 1;
        while (lIndex < size && data[lIndex ++] == trim);
        if (lIndex == size) return nullptr;
        while (rIndex >= lIndex && data[rIndex --] == trim);
        auto * result = new char [rIndex - lIndex];
        BufferTool().copy(data + lIndex, result, rIndex - lIndex);
        return result;
    }
    string& lstrip(string &str) {
        string::iterator p = find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(isspace)));
        str.erase(str.begin(), p);
        return str;
    }

    string& rstrip(string &str) {
        string::reverse_iterator p = find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(isspace)));
        str.erase(p.base(), str.end());
        return str;
    }

    string& strip(string &str) {
        lstrip(rstrip(str));
        return str;
    }
};

#endif //EFS_UTILITIES_H
