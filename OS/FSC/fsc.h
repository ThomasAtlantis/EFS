//
// Created by 12203 on 2019/6/26.
//

#ifndef FS_FILE_SYSTEM_CONTROLLER_H
#define FS_FILE_SYSTEM_CONTROLLER_H

#include "../../utilities.h"
#include "../params.h"
#include "../FBC/fbc.h"

/* *
 * 更新：
 * 1. 部分宏定义放入../params.h
 * 2. 更新数据结构INode和DirBlock
 * 3. 我改主意了：
      string _curUser; // 正在使用系统的用户名
      INode &_curINode; // 当前目录的i节点
      这两个的维护应该放到OS类中，FSC的这两个值由参数传入
 * */

#define _INODE_PADDING (_BLOCK_SIZE \
    - sizeof(time_t) * 2 \
    - sizeof(unsigned int) * 4 \
    - sizeof(bid_t) * 3 \
    - _FILENAME_MAXLEN - 1 \
    - (_USERNAME_MAXLEN - 1) * 2 \
    - 4)
#define _DIRBLOCK_ITEM_SIZE ((int)_BLOCK_SIZE / sizeof(bid_t))
#define _DIRBLOCK_PADDING (_BLOCK_SIZE - _DIRBLOCK_ITEM_SIZE)
#define _INDEXBLOCK_ITEM_SIZE _DIRBLOCK_ITEM_SIZE

namespace mode {
    enum accessMode {
        none = 0,
        read = 4,
        write = 2,
        exec = 1,
        total = 7
    };
}
namespace group {
    enum groupName {
        user, group, others
    };
}
// demo: iNode.mode[group::user] |= mode::read| mode::write;
// demo: iNode.mode[group::group] &= ~mode::read;
// demo: iNode.mode[group::others] = mode::none;


class FSController {
private:
    #pragma pack(1)
    typedef struct {
        time_t ctime; //创建时间
        time_t mtime; //修改时间
        unsigned int size; //文件大小
        unsigned int blocks; //文件所占的块数
        unsigned int bytes; // 最后一块字节数，size % blocks
        unsigned int childCount; // 目录文件用来记录文件数？？？？数组？？？
        bid_t bid; //i节点块号
        bid_t parentDir; //父文件夹I节点块号
        bid_t indexList[_BLOCK_INDEX_SIZE]; // 文件内容的块索引表
        char name [_FILENAME_MAXLEN + 1]; //文件名
        char owner[_USERNAME_MAXLEN + 1]; //文件所有者
        char group[_USERNAME_MAXLEN + 1]; //文件所有组
        char mode[3]; // 文件权限
        char type; // 文件类型
        char padding[_INODE_PADDING];
    } INode;

    typedef struct {
        bid_t itemList[_DIRBLOCK_ITEM_SIZE]; // 目录表项：下级文件的INode所在块号
        char padding[_DIRBLOCK_PADDING];
    } DirBlock, IndexBlock;
    #pragma pack()

    VHDController _vhdc; // 虚拟磁盘管理器
    FBController _fbc; // 空闲块管理器
    const bid_t _minBlockID; // 文件系统管理的最小块号
    const bid_t _maxBlockID; // 文件系统管理的最大块号

    /**
     * 获取新INode并初始化
     * @return INode *
     */
    INode * newINode() {
        auto * iNode = new INode;
        memset(&iNode->ctime, 0, sizeof(INode));
        return iNode;
    }

    /**
     * 获取新DirBlock并初始化
     * @return DirBlock *
     * IndexBlock的获取方法与之完全相同
     */
    DirBlock * newDirBlock() {
        auto dirBlock = new DirBlock;
        for (auto &item: dirBlock->itemList) item = _minBlockID;
        return dirBlock;
    }
    DirBlock * (*newIndexBlock)() = newDirBlock;
public:

    string partition; // 分区名

    /**
     * 文件系统构造函数
     * @param disk 虚拟硬盘文件名
     * @param minBlockID 管理的最小块号
     * @param maxBlockID 管理的最大块号
     * @param part 分区名
     */
    FSController(
        string disk,
        bid_t minBlockID,
        bid_t maxBlockID,
        string part):
        _vhdc(std::move(disk)),
        _minBlockID(minBlockID),
        _maxBlockID(maxBlockID),
        _fbc(_vhdc, minBlockID, maxBlockID),
        partition(std::move(part)){}
    /**
     * 获取当前路径名
     * @param curINode
     * @return 路径名
     * OS类传入方法：get((_fileSystem_x::INode *) curINode)
     * 其中curINode是OS维护的void*指针
     */
    string workingDir(INode * curINode) {
        return string(curINode->name);
    }

    /**
     * 判断目录文件是否为空
     * @param dirINode 目录文件的i节点
     * @return 操作成功与否
     */
    bool isEmptyDir(INode &dirINode) {
        return dirINode.childCount == 0;
    }

    /**
     * 获取指定INode的第i个块，使用混合多级索引
     * @param iNode i节点
     * @param i 索引表中的块序号
     * @return 目标块号
     */
    // TODO: 测试getBlockID
    bid_t * getBlockID(INode &iNode, int i) {
        int _12div = _1INDEX_NUM; // 1/2级索引分界
        int _23div = _1INDEX_NUM + _2INDEX_NUM * _INDEXBLOCK_ITEM_SIZE; // 2/3级索引分界
        int listSize = _INDEXBLOCK_ITEM_SIZE;
        int indexTotal = _23div + _3INDEX_NUM * listSize * listSize;
        if (i < _12div) return &iNode.indexList[i];
        else if (i < _23div) {
            int _2indexIndex = (i - _12div) / listSize;
            int _2indexOffset = (i - _12div) % listSize;
            IndexBlock indexBlock;
            _vhdc.readBlock((char *) &indexBlock, iNode.indexList[_12div + _2indexIndex]);
            return &indexBlock.itemList[_2indexOffset];
        } else if (i < indexTotal) {
            int _3indexIndex = (i - _23div) / (listSize * listSize);
            int _3indexIndexIndex = (i - _23div - _3indexIndex * listSize * listSize) / listSize;
            int _3indexOffset = (i - _23div - _3indexIndex * listSize * listSize) % listSize;
            IndexBlock indexBlock;
            _vhdc.readBlock((char *) &indexBlock, iNode.indexList[_23div + _2INDEX_NUM + _3indexIndex]);
            _vhdc.readBlock((char *) &indexBlock, indexBlock.itemList[_3indexIndexIndex]);
            return &indexBlock.itemList[_3indexOffset];
        } else return nullptr;
    }

    // TODO: 测试push
    bool push(INode &iNode, bid_t blockID) {
        bid_t * container = getBlockID(iNode, iNode.blocks);
        if (!container) return false;
        *container = blockID;
        iNode.blocks ++;
        return true;
    }

    // TODO: 测试pop
    bid_t * pop(INode &iNode) {
        if (iNode.blocks == 0) return nullptr;
        iNode.blocks --;
        return getBlockID(iNode, iNode.blocks);
    }

    /**
     * 根据文件名获取当前路径下指定文件的i节点
     * @param curINode 当前目录的i节点
     * @param fileName 文件名
     * @return 目标文件i节点
     */
    INode * getINode(INode * curINode, string fileName) {
        DirBlock * dirBlock = newDirBlock();
        INode * iNode = newINode();
        for (int i = 0; i < curINode->blocks; ++ i) {
            _vhdc.readBlock((char *)dirBlock, * getBlockID(*iNode, i));
            for (const auto &fileINode: dirBlock->itemList) {
                _vhdc.readBlock((char *)iNode, fileINode);
                if (string(iNode->name) == fileName) {
                    return iNode;
                }
            }
        }
        return nullptr;
    }

    /**在当前文件夹下创建子文件夹
     *@param curINode 当前目录
     * @param dirName 新建的文件夹名字
     *@return 新建文件夹的INode
     */
    INode * createDir(string dirName,INode * curINode) {
        INode * dirINode = newINode();
        // TODO: 改到这里了
        dirINode->name = dirName;
        dirINode->ctime = time(nullptr);
        dirINode->mtime = time(nullptr);
        dirINode->blocks = 1;
        dirINode->type = 'd';//目录文件
        dirINode->childCount = 0;
        if(!_fbc.distribute(dirINode->bid))
            std::cout<<"distribute unsuccessfully"<<endl;
        dirINode->parentDir = curINode.bid;
        INode parentINode {};
        _vhd.readBlock((char *) &parentINode, dirINode->parentDir);//找到父文件夹，写父文件夹的子文件块号
        DirBlock parentDirBlock{};
        _vhd.readBlock((char *) &parentDirBlock, parentINode->indexList[0]);//找到父文件夹，写父文件夹的子文件块号
        parentDirBlock.itemList[parentINode.childCount++]=dirINode->bid;
        _vhd.writeBlock((char *) &dirBlock, dirINode->indexList[0]);
        DirBlock * dirBlock = newDirBlock();//文件夹的目录表项，也就是文件夹i节点的内容
        _fbc.distribute(dirINode->indexList[0]);
        _vhd.writeBlock((char *) &parentDirBlock, parentINode->indexList[0]);
        push(dirINode,dirINode->bid);
        return dirINode;
    }
    bool writeDir(INode &dirINode,string dirName="")//修改dir信息，如文件夹名字或者子文件变更时用到
    {
        if(dirName!="")
            dirINode.name=dirName;
        dirINode.mtime=time(nullptr);

    }
    bool readDir()
    {

    }
    bool deleteF(INode &file)//输入此文件（夹）的I节点，删除此文件（夹）
    {
        if(isEmptyDir(file))
        {
            //找到父节点删除此节点信息
            //删除此节点
            INode parentINode {};
            _vhd.readBlock((char *) &parentINode,file.parentDir);
            DirBlock parentDirBlock{};
            _vhd.readBlock((char *) &parentDirBlock,parentINode.indexList[0]);
            for(int i=0;i<parentINode.childCount;i++)
                if(parentDirBlock.itemList[i]==file.bid)//在父节点删除此文件的I节点块号
                {
                    for(int j=i;j<parentINode.childCount-1;j++)
                        parentDirBlock.itemList[j]=parentDirBlock.itemList[j+1];
                    break;
                }
            parentINode.childCount--;

            //回收所有的文件块

            //回收此文件I节点
            _vhd.recycle(file.bid);
        }
        else
        {
            // 递归删除此文件夹下的所有子文件
            _vhd.readBlock();
            for(int i=0;i<file.childCount;i++)
                deleteF();
        }
    }
    bool havePower(User user,char type)//判断用户是否具有对应的权限
    {
        ifstream userTab(USER_FILE_NAME);//上面宏定义需要改
        //判断用户是否具有权限

        return false;
    }

    bool openFile(string fileName,INode &iNode)//根据文件名打开文件，返回文件的Inode信息标识
    {
        cout<<"in openFile()"<<endl;
        iNode->parentDir = curINode.bid;//如何获得？？？
        DirBlock parentDirBlock{};//读父文件夹的子文件I节点块
        _vhd.readBlock((char *) &parentDirBlock,curINode.indexList[0]);

        for(int i=0;i<curINode.childCount;i++)
        {
            INode temp{};
            _vhd.readBlock((char *) &temp,parentDirBlock.itemList[i]);
            if(temp.name==fileName)
            {
                iNode=temp;
                break;
            }
            if(i==curINode.childCount-1)
                return false;
        }
        return true;
    }
    bool createFile(string fileName,iNode &inode,char owner[6]="",string content="")//创建文件
    {
        cout<<"in createFile()"endl;
        if(fileName.length()>_FILENAME_MAXLEN)
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
            unsigned int indexList=(content.length()-1)/BLOCK_CONTENT_SIZE + 1;
            if(indexList==1)//直接索引
            {
                inode.addr1=0;
                inode.addr2=0;
                if(_fbc.distribute(inode.indexList))
                {
                    _vhdc.writeBlock((char*) &content,bid[i]);
                }
                else
                {
                    _fbc.recycle(inode.indexList);
                    return false;
                }
            }
            else if(indexList<=BLOCK_CONTENT_SIZE/4)//一次间址
            {
                int bid[indexList];
                inode.addr1=1;
                inode.addr2=0;
                for(int i=0;i<indexList;i++)//分配块，如果失败将分配的块回收
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
            else if(indexList<=(BLOCK_CONTENT_SIZE/4)*(BLOCK_CONTENT_SIZE/4))//二次间址
            {

            }
            inode.indexList=indexList;
            //写inode
            _vhdc.writeBlock((char*) &inode,inode.bid);
            cout<<"the file is be created successfully."<<endl;
        }
        else
            std::cout<<"the file already exists,please give a different filename! "<<endl;
    }


};


#endif //FS_FILE_SYSTEM_CONTROLLER_H
