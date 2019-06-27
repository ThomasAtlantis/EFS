//
// Created by 12203 on 2019/6/26.
//
#include "../FSC/fsc.h"

int main()
{
    FileSystemController fsc("wgq");
    iNode inode;
    fsc.createFile("123",inode,"wgq1","12345678998778999");
}
