//
// Created by 12203 on 2019/6/26.
//

#ifndef FS_FILE_SYSTEM_CONTROLLER_H
#define FS_FILE_SYSTEM_CONTROLLER_H

#include <iostream>
#include <fstream>
#include <ctime>
#include <utility>
#include <vector>
#include <string>
#include <algorithm>
#include "../../VM/tiny_efs.h"
#include "params.h"

using std::string;
using std::cout;
using std::cin;
using std::endl;
using std::vector;

#define USER_FILE_NAME ./.....
#define FILENAME_MAXLEN 19
#define MAX_SUB 10
#define ROOT_BLOCK_NUM 1
typedef char User[];
typedef size_t bid_t;
/*
 * 问题：
 * 1. 确定两个数据结构 inode和dir
 * 2. 如何获取当前的路径/当前的inode
 *
 *
 *
 *
 *
 * */

typedef struct//sizeof(iNode)=
{
    time_t ctime;//创建时间-4
    time_t mtime;//修改时间-4
    unsigned int size;//文件大小-4
    unsigned int blocks;//文件所占的块数-4
    int addr1;//一个一次间址-4
    int addr2;//一个二次间址-4
    bid_t bid;//i节点所在的块号-4
    bid_t blockNum[4];//所指的文件内容的块号-16
    char name[FILENAME_MAXLEN + 1];//文件名-20
    char owner[6];//文件所有者-6
    char group[6];//文件所有组-6
    char mode;//文件类别或权限-1
    int parfDir;//父文件夹块号-4
}iNode;
typedef struct
{
    int index;//块号
    char name[FILENAME_MAXLEN + 1];//文件名(当前目录)
    int parfiNode;//父目录节点
    int subFile[(BLOCK_SIZE-28)/4];//子文件/夹 i节点块号
}DIRHead;//目录项

void initDIRHead(DIRHead  &dir)//DIRHead 初始化
{
    for(int i=0;i<(BLOCK_SIZE-28)/4;i++)
        dir.subFile[i]=-1;
}

typedef struct
{
    int index;
    int subFile[(BLOCK_SIZE-4)/4];
}DIRData;


class FileSystemController
{
private:
    User user; //用户名
    FBController &_fbc;
    VHDController &_vhdc;
    INode * curINode;
public:
    FileSystemController(User userName)
    {
        user=userName;
        curINode = new INode;
        curINode->name = "~";
    }
    string getPathNow()//获得当前路径目录
    {

    }
    iNode getPathiNode()//获得当前路径目录i节点
    {

    }
    bool isEmptyDIR(DIRHead &dir)//判断此文件夹是否为空
    {
        for(int i=0;i<(BLOCK_SIZE-28)/4;i++)
            if(!dir.subFile[i]==-1)
                return false;
        return true;
    }
    bool createDIR(DIRHead &dir,string dirName)
    {
        initDIRHead(dir);
        dir.name=dirName;
        //得到当前父文件夹i节点,并修改
        iNode inode = getPathiNode();

        //strcpy(dir.parfNmae,pathNow.c_str());
        if(_fbc.distribute(dir.index))
        {
            _vhdc.writeBlock((char*) &dir,dir.index);
        }

        //找到父文件夹，写父文件夹的子文件块号
    }
    bool writeDIR(DIRHead &dir,string dirName="")//修改dir信息，如文件夹名字或者子文件变更时用到
    {

    }
    bool readDIR()
    {

    }
    bool deleteDIR(DIR &dir)
    {
        if(isEmptyDIR(dir))
        {
            //找到父节点删除此节点信息
            //删除此节点
        }
        else
        {
            //删除此文件夹下的所有子文件
        }
    }
    bool havePower(User user,char type)//判断用户是否具有对应的权限
    {
        ifstream userTab(USER_FILE_NAME);//上面宏定义需要改
        //判断用户是否具有权限

        return false;
    }

    bool isExist(string fileName)//判断是否已经存在此文件
    {
        //获取当前文件夹的dir，遍历他的subFile，看是否有文件名
    }
    string openFile(string fileName)//根据文件名打开文件，返回文件的信息标识
    {
        cout<<"in openFile()"<<endl;
        //读出vector索引index
        //得到当前的路径pathNow
        path=pathNow+"/"+fileName;
        int num;//i节点所在块标号
        for(vector<vector <string,int> >::iterator it=index.begin();it!=index.end();++it)
        {
            if(it[0]==path)
            {
                num=it[1];
                break;
            }
        }
    }
    bool createFile(string fileName,iNode &inode,char owner[6]="",string content="")//创建文件
    {
        cout<<"in createFile()"endl;
        if(fileName.length()>FILENAME_MAXLEN)
        {
            cout<<"sorry, the length of filename is too long.(please no mre than char[20].)"<<endl;
            return false;
        }
        if(!isExist(fileName))//重名等其他判断
        {
            //申请块，I节点
            bid_t blockID;
            if(!_fbc.distribute(blockID))
            {
                cout<<"distribute inode block failed!"<<endl;
                return false;
            }
            inode.ctime=inode.mtime=time(nullptr);
            inode.bid=blockID;
            inode.owner=owner;
            inode.name=fileName;

            inode.size=content.length();
            unsigned int blockNum=(content.length()-1)/BLOCK_CONTENT_SIZE + 1;
            if(blockNum==1)//直接索引
            {
                inode.addr1=0;
                inode.addr2=0;
                if(_fbc.distribute(inode.blockNum))
                {
                    _vhdc.writeBlock((char*) &content,bid[i]);
                }
                else
                {
                    _fbc.recycle(inode.blockNum);
                    return false;
                }
            }
            else if(blockNum<=BLOCK_CONTENT_SIZE/4)//一次间址
            {
                int bid[blockNum];
                inode.addr1=1;
                inode.addr2=0;
                for(int i=0;i<blockNum;i++)//分配块，如果失败将分配的块回收
                {
                    bool success=true;
                    if(_fbc.distribute(bid[i]))
                    {
                        string temp=content.substr(i*BLOCK_CONTENT_SIZE,BLOCK_CONTENT_SIZE);//提取子串,从i*BLOCK_CONTENT_SIZE开始,提取BLOCK_CONTENT_SIZE个字节
                        _vhdc.writeBlock((char*) &temp,bid[i]);
                    }
                    else//分配失败
                    {
                        for (int j = i-1; j >= 0; j--) {
                            _fbc.recycle(bid[j]);
                        }
                        success=false;
                    }
                    if(!success)
                        break;
                }
            }
            else if(blockNum<=(BLOCK_CONTENT_SIZE/4)*(BLOCK_CONTENT_SIZE/4))//二次间址
            {

            }
            inode.blockNum=blockNum;
            //写inode
            _vhdc.writeBlock((char*) &inode,inode.bid);
            cout<<"the file is be created successfully."<<endl;
        }
        else
            std::cout<<"the file already exists,please give a different filename! "<<endl;
    }
    iNode openFile(string fileName)//根据文件名返回文件i节点
    {

    }

    bool deleteFile(string fileName)//删除文件
    {
        if(isExist(fileName))
        {
            iNode inode=openFile(fileName);
            //删除操作
        }
        else
        {
            cout<<"sorry, the file is not existed!"<<endl;
        }
    }


};




#endif //FS_FILE_SYSTEM_CONTROLLER_H
