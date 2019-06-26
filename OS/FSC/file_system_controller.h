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

using std::string;
using std::cout;
using std::cin;
using std::endl;
using std::vector;

#define USER_FILE_NAME ./.....
#define User char[]
#define FILENAME_MAXLEN 20
#define MAX_SUB 10
#define ROOT_BLOCK_NUM 1
typedef size_t bid_t;

typedef struct
{
    time_t ctime;//创建时间
    time_t mtime;//修改时间
    unsigned int size;//文件大小
    unsigned int blocks;//文件所占的块数
    int addr1;//一个一次间址
    int addr2;//一个二次间址
    bid_t bid;//i节点所在的块号
    bid_t blockNum;//所指的文件内容的块号
    char name[FILENAME_MAXLEN + 1];//文件名
    char owner[6];//文件所有者
    char group[6];//文件所有组
    char mode;//文件类别或权限
    int parfDir;//父文件夹块号
}iNode;

typedef struct {
    int index;//块号
    char name[FILENAME_MAXLEN + 1];//文件名(当前目录)
    char parfNmae[FILENAME_MAXLEN + 1];//父目录名
    int subFile[MAX_SUB];//子文件/夹 i节点块号，最多十个
} DIRHead;

typedef struct {
    int index;//块号
    int subFile[MAX_SUB];//子文件/夹 i节点块号，最多十个
} DIRData;//目录项


class FileSystemController
{
private:
    User user; //用户名
    FBController &_fbc;
    VHDController &_vhdc;
public:
    FileSystemController(User userName)
    {
        user=userName;
    }
    bool isEmptyDIR(DIR &dir)//判断此文件夹是否为空
    {
        for(int i=0;i<MAX_SUB;i++)
            if(!dir.subFile[i]==-1)
                return false;
        return true;
    }
    bool createDIR(DIR &dir,string dirName)
    {
        dir.name=dirName;
        //得到当前文件夹路径 pathNow
        strcpy(dir.parfNmae,pathNow.c_str());
        if(_fbc.distribute(dir.index))
        {
            _vhdc.writeBlock((char*) &dir,dir.index);
        }

        //找到父文件夹，写父文件夹的子文件块号
    }
    bool writeDIR()
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
            /*int bid[blockNum];
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
            }*/
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
    bool amendFile(string fileName)//修改文件，包括文件内容的增添与修改,
    //假设文件已经打开且具有修改权限
    {
        if(isExist(fileName))
        {
            iNode inode=openFile(fileName);
            if(inode.addr1==0)//直接索引
            {
                //将文件显示出来并修改，写会磁盘
            }
            else if(inode.addr1==1)//一次间址
            {

            }
        }
        else
        {
            cout<<"the file is not existed!"<<endl;
        }
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
