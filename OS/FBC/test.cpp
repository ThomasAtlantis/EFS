//
// Created by MAC on 2019/6/26.
//

#include "fbc.h"

int main() { // demo

    bid_t blockID;
    char buff_w[_BLOCK_SIZE];
    char buff_r[_BLOCK_SIZE];

    // 初始化虚拟磁盘与空闲块管理器
    VHDController vhdc("./SLOT_2/data.vhd"); // 相对于exe的硬盘文件相对路径
    FBController fbc(vhdc, 0, 999); // 虚拟磁盘、管理最小块号、管理最大块号
    fbc.formatting(); // 格式化硬盘[可选]

    // 读写字符串demo
    string str = R"(hello world!)"; // R"()"是C++11的raw字符串
    BufferTool().copy(buff_w, str.c_str(), 13); // 在utilities.h中的工具函数
    fbc.distribute(blockID); // 分配一个空闲块
    cout << blockID << endl;
    vhdc.writeBlock(buff_w, blockID); // 写块
    vhdc.readBlock(buff_r, blockID); // 读块
    cout << buff_r << endl;
    fbc.recycle(blockID); // 回收空闲块

    // 读写结构体demo
    // PADDING的宏要自己写，不要以下划线开头
    #define DEMO_PADDING (_BLOCK_SIZE \
        - sizeof(char) * 22 \
        - sizeof(int) * 1 \
        - sizeof(float) * 1)
    #pragma pack(1)
    typedef struct {
//        string name; // string也不是不可以，不好
        char name[21];
        char sex;
        int age;
        float grade;
        char padding[DEMO_PADDING];
    } Demo;
    #pragma pack()
    Demo demoStructW = {"Thomas", 'M', 21, 4.99};
    Demo demoStructR {};
    fbc.distribute(blockID);
    vhdc.writeBlock((char *) &demoStructW, blockID); // (char *) &是核心用法
    vhdc.readBlock((char *) &demoStructR, blockID); // 但如果demoStructR本身是指针，不要加&
    fbc.recycle(blockID);
    cout << demoStructR.name<< " "
         << demoStructR.sex << " "
         << demoStructR.age << " "
         << demoStructR.grade << endl;
    return 0;
}