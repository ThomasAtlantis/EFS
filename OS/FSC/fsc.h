//
// Created by 12203 on 2019/6/26.
//

#ifndef EFS_FSC_H
#define EFS_FSC_H

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
namespace fio {
    enum operateType {
        in = 1,
        out = 2,
        app = 4,
        bin = 8
    };
}
// 用法相似，参考group
typedef struct {
    char * data;
    size_t size;
} Buffer;

class FSController {
private:
    const bid_t _minBlockID; // 文件系统管理的最小块号
    const bid_t _maxBlockID; // 文件系统管理的最大块号
public:
    VHDController _vhdc; // 虚拟磁盘管理器
    FBController _fbc; // 空闲块管理器
    string partition; // 分区名
    #pragma pack(1)
    typedef struct {
        time_t ctime; //创建时间
        time_t mtime; //修改时间
        unsigned int size; //文件大小
        unsigned int blocks; //文件所占的块数
        unsigned int bytes; // 最后一块字节数，size % blocks
        unsigned int childCount; // 目录文件用来记录文件数
        bid_t bid; //i节点块号
        bid_t parentDir; //父文件夹I节点块号
        bid_t indexList[_BLOCK_INDEX_SIZE]; // 文件内容的块索引表
        char name [_FILENAME_MAXLEN + 1]; //文件名
        char owner[_USERNAME_MAXLEN + 1]; //文件所有者
        char group[_USERNAME_MAXLEN + 1]; //文件所有组
        char mode[3]; // 文件权限
        char type; // 文件类型: 'L'链接文件，'D'目录文件，'F'普通文件
        char padding[_INODE_PADDING];
    } INode;

    INode * rootINode;

    typedef struct {
        bid_t itemList[_DIRBLOCK_ITEM_SIZE]; // 目录表项：下级文件的INode所在块号
        char padding[_DIRBLOCK_PADDING];
    } DirBlock, IndexBlock;
    #pragma pack()

    typedef struct {
        INode * iNode;
        char type;
    } FilePointer;

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
        string partName,
        string userName):
        _vhdc(std::move(disk)),
        _minBlockID(minBlockID),
        _maxBlockID(maxBlockID),
        _fbc(_vhdc, minBlockID, maxBlockID),
        partition(std::move(partName)) {
        rootINode = createRootDir(partName, userName);
    }

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
    bid_t * getBlockID(INode &iNode, int i) {
        const int _1indexNumb = _1INDEX_NUM;
        const int _2indexNumb = _2INDEX_NUM;
        const int _3indexNumb = _3INDEX_NUM;
        const int _2indexSize = _INDEXBLOCK_ITEM_SIZE;
        const int _3indexSize = _2indexSize * _2indexSize;
        const int _2_3indexDiv = _1indexNumb + _2indexNumb * _2indexSize;
        IndexBlock indexBlock;
        if (i < _1indexNumb) return &iNode.indexList[i];
        else if (i < _2_3indexDiv) {
            int index  = (i - _1indexNumb) / _2indexSize;
            int offset = (i - _1indexNumb) % _2indexSize;
            _vhdc.readBlock((char *) &indexBlock, iNode.indexList[_1indexNumb + index]);
            return new bid_t(indexBlock.itemList[offset]);
        } else if (i < _2_3indexDiv + _3indexNumb * _3indexSize) {
            int index  = (i - _2_3indexDiv) / (_3indexSize);
            int index2 = (i - _2_3indexDiv - index * _3indexSize) / _2indexSize;
            int offset = (i - _2_3indexDiv - index * _3indexSize) % _2indexSize;
            _vhdc.readBlock((char *) &indexBlock, iNode.indexList[_1indexNumb + _2indexNumb + index]);
            _vhdc.readBlock((char *) &indexBlock, indexBlock.itemList[index2]);
            return new bid_t(indexBlock.itemList[offset]);
        } else return nullptr;
    }

    /**
     * 设置指定INode的第i个块，使用混合多级索引
     * @param iNode i节点
     * @param i 索引表中的块序号
     * @param blockID 块号
     * @return 目标块号
     */
    bool setBlockID(INode &iNode, int i, bid_t blockID) {
        const int _1indexNumb = _1INDEX_NUM;
        const int _2indexNumb = _2INDEX_NUM;
        const int _3indexNumb = _3INDEX_NUM;
        const int _2indexSize = _INDEXBLOCK_ITEM_SIZE;
        const int _3indexSize = _2indexSize * _2indexSize;
        const int _2_3indexDiv = _1indexNumb + _2indexNumb * _2indexSize;
        IndexBlock indexBlock;
        if (i >= _2_3indexDiv + _3indexNumb * _3indexSize) return false;
        else if (i < _1indexNumb) iNode.indexList[i] = blockID;
        else if (i < _2_3indexDiv) {
            int index  = (i - _1indexNumb) / _2indexSize;
            int offset = (i - _1indexNumb) % _2indexSize;
            _vhdc.readBlock((char *) &indexBlock, iNode.indexList[_1indexNumb + index]);
            indexBlock.itemList[offset] = blockID;
            _vhdc.writeBlock((char *) &indexBlock, iNode.indexList[_1indexNumb + index]);
        } else {
            int index  = (i - _2_3indexDiv) / (_3indexSize);
            int index2 = (i - _2_3indexDiv - index * _3indexSize) / _2indexSize;
            int offset = (i - _2_3indexDiv - index * _3indexSize) % _2indexSize;
            _vhdc.readBlock((char *) &indexBlock, iNode.indexList[_1indexNumb + _2indexNumb + index]);
            bid_t indexSav = indexBlock.itemList[index2];
            _vhdc.readBlock((char *) &indexBlock, indexSav);
            indexBlock.itemList[offset] = blockID;
            _vhdc.writeBlock((char *) &indexBlock, indexSav);
        }
        return true;
    }

    /**
     * i节点索引表进栈
     * @param iNode i节点
     * @param blockID 放入栈的块号
     * @return 操作成功与否
     */
    bool push(INode &iNode, bid_t blockID) {
        const int _1indexNumb = _1INDEX_NUM;
        const int _2indexNumb = _2INDEX_NUM;
        const int _3indexNumb = _3INDEX_NUM;
        const int _2indexSize = _INDEXBLOCK_ITEM_SIZE;
        const int _3indexSize = _2indexSize * _2indexSize;
        const int _2_3indexDiv = _1indexNumb + _2indexNumb * _2indexSize;
        bid_t indexBlockID;
        if (iNode.blocks >= _1indexNumb) { // 栈顶指针超出直接索引区
            if (iNode.blocks < _2_3indexDiv) {
                int index  = (iNode.blocks - _1indexNumb) / _2indexSize;
                int offset = (iNode.blocks - _1indexNumb) % _2indexSize;
                if (offset == 0) {
                    if (!_fbc.distribute(indexBlockID)) return false;
                    iNode.indexList[_1indexNumb + index] = indexBlockID;
                }
            } else if (iNode.blocks < _2_3indexDiv + _3indexNumb * _3indexSize) {
                int index  = (iNode.blocks - _2_3indexDiv) / _3indexSize;
                int offset = (iNode.blocks - _2_3indexDiv) % _3indexSize;
                int index2 = (iNode.blocks - _2_3indexDiv - index * _3indexSize) / _2indexSize;
                int offset2= (iNode.blocks - _2_3indexDiv - index * _3indexSize) % _2indexSize;
                IndexBlock * indexBlock = newDirBlock();
                bid_t & indexBlockID2 = iNode.indexList[_1indexNumb + _2indexNumb + index];
                if (offset == 0) {
                    if (!_fbc.distribute(indexBlockID)) return false;
                    indexBlock->itemList[0] = indexBlockID;
                    if (!_fbc.distribute(indexBlockID)) return false;
                    _vhdc.writeBlock((char *) indexBlock, indexBlockID);
                    indexBlockID2 = indexBlockID;
                } else if (offset2 == 0) {
                    if (!_fbc.distribute(indexBlockID)) return false;
                    _vhdc.readBlock((char *) indexBlock, indexBlockID2);
                    indexBlock->itemList[index2] = indexBlockID;
                    _vhdc.writeBlock((char *) indexBlock, indexBlockID2);
                }
            }
        }
        if (setBlockID(iNode, iNode.blocks, blockID)) {
            iNode.blocks ++;
            return true;
        } else return false;
    }
    /**
     * i节点索引表弹栈
     * @param iNode i节点
     * @return 块号
     */
    bid_t * pop(INode &iNode) {
        const int _1indexNumb = _1INDEX_NUM;
        const int _2indexNumb = _2INDEX_NUM;
        const int _3indexNumb = _3INDEX_NUM;
        const int _2indexSize = _INDEXBLOCK_ITEM_SIZE;
        const int _3indexSize = _2indexSize * _2indexSize;
        const int _2_3indexDiv = _1indexNumb + _2indexNumb * _2indexSize;
        bid_t indexBlockID;
        if (iNode.blocks >= _1indexNumb) { // 栈顶指针超出直接索引区
            if (iNode.blocks < _2_3indexDiv) {
                int index  = (iNode.blocks - _1indexNumb) / _2indexSize;
                int offset = (iNode.blocks - _1indexNumb) % _2indexSize;
                if (offset == 0) {
                    _fbc.recycle(iNode.indexList[_1indexNumb + index]);
                }
            } else if (iNode.blocks < _2_3indexDiv + _3indexNumb * _3indexSize) {
                int index  = (iNode.blocks - _2_3indexDiv) / _3indexSize;
                int offset = (iNode.blocks - _2_3indexDiv) % _3indexSize;
                int index2 = (iNode.blocks - _2_3indexDiv - index * _3indexSize) / _2indexSize;
                int offset2= (iNode.blocks - _2_3indexDiv - index * _3indexSize) % _2indexSize;
                IndexBlock * indexBlock = newDirBlock();
                bid_t & indexBlockID2 = iNode.indexList[_1indexNumb + _2indexNumb + index];
                if (offset == 0 || offset2 == 0) {
                    _vhdc.readBlock((char *) indexBlock, indexBlockID2);
                    _fbc.recycle(indexBlock->itemList[0]);
                    if (offset == 0) _fbc.recycle(indexBlockID2);
                }
            }
        }
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
    //TODO: test getINode
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

    // TODO: 路径解析
    INode * parsePath(INode * curINode, string fileName) {
        return getINode(curINode, std::move(fileName));
    }

    /**
     *
     * 此文件夹下是否存在指定文件
     * @param iNode
     * @param fileName
     * @return
     */
    bool exists(INode * iNode, string fileName) {
        return (nullptr != getINode(iNode, std::move(fileName)));
    }

    /**在当前文件夹下创建子文件夹
     *@param curINode 当前目录
     * @param dirName 新建的文件夹名字
     *@return 新建文件夹的INode
     */
    // TODO :测试createDir
    INode * createDir(INode * curINode, string curUser, string dirName) {
        if (dirName.length() > _FILENAME_MAXLEN)
            return nullptr;
        INode * dirINode = newINode();
        strcpy(dirINode->name, dirName.c_str());
        dirINode->ctime = time(nullptr);
        dirINode->mtime = time(nullptr);
        dirINode->blocks = 1;
        dirINode->type = 'D';//目录文件
        dirINode->childCount = 0;
        BufferTool().copy(dirINode->mode, curINode->mode, 3);
        strcpy(dirINode->owner,curUser.c_str());
        if(!_fbc.distribute(dirINode->bid))
        {
            std::cout<<"distribute unsuccessfully"<<endl;
            return nullptr;
        }
        dirINode->parentDir = curINode->bid;
        addDirChild(*curINode, *dirINode);
        bid_t num;
        _fbc.distribute(num);
        push(*dirINode,num);
        DirBlock * dirBlock = newDirBlock();
        _vhdc.writeBlock((char *) dirBlock ,num);
        _vhdc.writeBlock((char *) dirINode,dirINode->bid);
        return dirINode;
    }

    /**
     * 创建根目录文件
     * @param dirName 分区名称
     * @param curUser 创建分区的用户
     * @return 根目录文件的i节点
     */
     // TODO: test createRootDir
    INode * createRootDir(string dirName, string curUser) {
        if (dirName.length() > _FILENAME_MAXLEN)
            return nullptr;
        INode * dirINode = newINode();
        strcpy(dirINode->name, dirName.c_str());
        dirINode->ctime = time(nullptr);
        dirINode->mtime = time(nullptr);
        dirINode->blocks = 1;
        dirINode->type = 'D';//目录文件
        dirINode->childCount = 0;
        dirINode->mode[group::user] = mode::total;
        dirINode->mode[group::group] = mode::total;
        dirINode->mode[group::others] = mode::total;
        strcpy(dirINode->owner,curUser.c_str());
        if(!_fbc.distribute(dirINode->bid))
        {
            std::cout<<"distribute unsuccessfully"<<endl;
            return nullptr;
        }
        dirINode->parentDir = _minBlockID;
        bid_t num;
        _fbc.distribute(num);
        push(*dirINode,num);
        DirBlock * dirBlock = newDirBlock();
        _vhdc.writeBlock((char *) dirBlock ,num);
        _vhdc.writeBlock((char *) dirINode,dirINode->bid);
        return dirINode;
    }

    /**
     *
     * 改文件名字或者所有者
     * @param dirINode
     * @param dirName
     * @return
     */
     // TODO: test writeDir
    bool writeDir(INode &dirINode,string dirName="",string  curUser="" ) { //修改dir信息，如文件夹名字或者子文件变更时用到
        if(!dirName.empty())
            strcpy(dirINode.name,dirName.c_str());
        if(!curUser.empty())
            strcpy(dirINode.owner,curUser.c_str());
        dirINode.mtime=time(nullptr);
        _vhdc.writeBlock((char *) & dirINode, dirINode.bid);
        return true;
    }

    /**
     * 添加文件到文件夹
     * @param dirINode 目录文件的i节点
     * @param child 子文件的i节点
     * @return 操作成功与否
     */
     //TODO: test addDirChild
    bool addDirChild(INode &dirINode, INode &child) {
//         if (!accessible(dirINode, mode::write)) return false;
        DirBlock * dirBlock = newDirBlock();
        int index = dirINode.childCount / _DIRBLOCK_ITEM_SIZE;
        int offset = dirINode.childCount % _DIRBLOCK_ITEM_SIZE;
        bid_t * blockID = getBlockID(dirINode, index);
        if (!blockID) {
            if (!_fbc.distribute(* blockID))
                return false;
            push(dirINode, * blockID);
        } else {
            _vhdc.readBlock((char *) dirBlock, * blockID);
        }
        dirBlock->itemList[offset]=child.bid;
        dirINode.childCount ++;
        _vhdc.writeBlock((char *) dirBlock, * blockID);
        return true;
    }
    /**
     *
     * 此函数没有考虑文件夹具有多个块的子文件I节点
     * 把子文件I节点从父文件夹移除，子文件I节点以及子文件没有删除
     * @param dirINode
     * @param child
     * @return
     */
     // TODO: test removeDirChild
    bool removeDirChild(INode &dirINode, INode &child) {
//         if (!accessible(dirINode, mode::write)) return false;
        DirBlock * dirBlock = newDirBlock();
        int index = dirINode.childCount / _DIRBLOCK_ITEM_SIZE;
        int offset = dirINode.childCount % _DIRBLOCK_ITEM_SIZE;
        bid_t * blockID = getBlockID(dirINode, index);
        _vhdc.readBlock((char *) dirBlock, * blockID);
        for(int i=0;i<offset;i++)
        {
            if(dirBlock->itemList[i]==child.bid)
            {
                for(int j=i;j<offset-1;j++)
                {
                    dirBlock->itemList[j]=dirBlock->itemList[j+1];
                }
            }
        }
        dirINode.childCount --;
        _vhdc.writeBlock((char *) & dirINode, dirINode.bid);
        return true;
    }
    /**
     * 返回文件夹的子文件i节点vector
     * @param dirINode
     * @return
     */
    // TODO: test listDir
    vector<INode *> listDir(INode &dirINode) {
//        if (!accessible(dirINode, mode::read)) return {};
        vector <INode *> result;
        if(dirINode.type=='F'||(dirINode.type=='D'&&isEmptyDir(dirINode))) // 文件或空文件夹
        {
            return {};
        }
        else//正常含有子文件的文件夹
        {
            for(int i=0;i<dirINode.blocks-1;i++)
            {
                auto * blockID  = getBlockID(dirINode, i) ;
                DirBlock * dirBlock = newDirBlock();
                _vhdc.readBlock((char *) dirBlock,*blockID);
                for(int j=0;j<_DIRBLOCK_ITEM_SIZE;j++)
                {
                    INode * iNode = newINode();
                    _vhdc.readBlock((char *) iNode,dirBlock->itemList[j]);
                    result.push_back(iNode);
                }
            }
            int offset = dirINode.childCount % (_DIRBLOCK_ITEM_SIZE + 1);
            bid_t * blockID = getBlockID(dirINode, dirINode.blocks - 1);
            DirBlock * dirBlock = newDirBlock();
            _vhdc.readBlock((char *) dirBlock, *blockID);
            for(int i=0;i<offset;i++)
            {
                INode * iNode = newINode();
                _vhdc.readBlock((char *) iNode,dirBlock->itemList[i]);
                result.push_back(iNode);
            }
        }
        return result;
    }

    // TODO: test removeFile
    bool removeFile(INode &file) { //输入此文件（夹）的I节点，删除此文件（夹）
//        if (!accessible(file, mode::write)) return false;
        if(file.type=='D'&&isEmptyDir(file))//空的文件夹
        {
            //找到父节点删除此节点信息
            //删除此节点
            INode * parentINode = newINode();
            _vhdc.readBlock((char *) parentINode,file.parentDir);
            removeDirChild(*parentINode, file);
            bid_t * blockID = pop(file);
            _fbc.recycle(* blockID);//回收此节点块和此文件块
            _fbc.recycle(file.bid);
        }
        else if(file.type=='D')//非空文件夹
        {
            // 递归删除此文件夹下的所有子文件
            for(int i=0;i<file.blocks-1;i++)
            {
                bid_t * blockID = pop(file);
                DirBlock * dirBlock = newDirBlock();
                _vhdc.readBlock((char *) dirBlock,*blockID);
                _fbc.recycle(* blockID);
                for(int j=0;j<_DIRBLOCK_ITEM_SIZE;j++)
                {
                    INode * dirINode = newINode();
                    _vhdc.readBlock((char *) dirINode,dirBlock->itemList[j]);
                    removeFile(*dirINode);
                }
            }
            int offset = file.childCount % (_DIRBLOCK_ITEM_SIZE + 1);
            bid_t * blockID = pop(file);
            DirBlock * dirBlock = newDirBlock();
            _vhdc.readBlock((char *) dirBlock,*blockID);
            _fbc.recycle(* blockID);
            for(int i=0;i<offset;i++)
            {
                INode * dirINode = newINode();
                _vhdc.readBlock((char *) dirINode,dirBlock->itemList[i]);
                removeFile(*dirINode);
            }
            removeFile(file);
        }
        else if(file.type=='F')//普通文件
        {
            INode * parentINode = newINode();
            _vhdc.readBlock((char *) parentINode,file.parentDir);
            removeDirChild(*parentINode,file);
            for(int i=0;i<file.blocks;i++)
            {
                bid_t * blockID = pop(file);
                _fbc.recycle(* blockID);//回收此节点块和此文件块
            }
            _fbc.recycle(file.bid);
        }
        return true;
    }



    //TODO: test createFile
    INode * createFile(INode * curINode, string fileName, string curUser) { //创建文件
//        if (!accessible(*curINode, mode::write)) return nullptr;
//        cout << "in createFile()" << endl;
        INode * iNode = newINode();
        if(fileName.length()>_FILENAME_MAXLEN)
        {
            cout<<"sorry, the length of filename is too long.(please no mre than char[20].)"<<endl;
            return nullptr;
        }
        if(!exists(curINode, fileName))//重名等其他判断
        {
            //申请块，I节点
            bid_t blockID;
            if(!_fbc.distribute(blockID))
            {
                cout<<"distribute inode block failed!"<<endl;
                return nullptr;
            }
            iNode->ctime=iNode->mtime=time(nullptr);
            iNode->bid=blockID;
            strcpy(iNode->owner,curUser.c_str());
            iNode->type='F';
            strcpy(iNode->name,fileName.c_str());
            BufferTool().copy(iNode->mode, curINode->mode, 3);
            iNode->size=0;
            bid_t * firstBlock = new bid_t;
            if(!_fbc.distribute(*firstBlock))
            {
                cout<<"distribute block failed!"<<endl;
                return nullptr;
            }
            push(*iNode,*firstBlock);
            _vhdc.writeBlock((char *)iNode,iNode->bid);
        }
        else
            std::cout<<"the file already exists,please give a different filename! "<<endl;
        return iNode;
    }

    // TODO: 加入权限判断，将用户表传进来
    FilePointer * openFile(INode * curINode, string fileName, char type) { //根据文件名打开文件
//        cout<<"in openFile()"<<endl;
//        if ((type & fio::in) && !accessible(*curINode, mode::read)) return nullptr;
//        if ((type & fio::out) && !accessible(*curINode, mode::write)) return nullptr;
        INode * iNode = getINode(curINode, std::move(fileName));
        if(iNode) {
            auto * filePointer = new FilePointer;
            filePointer->iNode = iNode;
            filePointer->type = type;
            return filePointer;
        }
        else
        {
            cout<<"the file is not existed!"<<endl;
        }
        return nullptr;
    }

    Buffer * readFile(FilePointer fp) {
        if (!(fp.type & fio::in) || fp.iNode->blocks == 0) return nullptr;
        auto * buffer = new Buffer;
        buffer->data = new char [fp.iNode->blocks * _BLOCK_SIZE];
        buffer->size = fp.iNode->size;
        char * p = buffer->data;
        for (int i = 0; i < fp.iNode->blocks - 1; i ++) {
            bid_t * blockNum = getBlockID(*fp.iNode, i);
            _vhdc.readBlock(p, *blockNum);
            p += _BLOCK_SIZE;
        }
        char block[_BLOCK_SIZE];
        bid_t * blockID = getBlockID(*fp.iNode, fp.iNode->blocks - 1);
        _vhdc.readBlock(block, *blockID);
        BufferTool().copy(p, block, fp.iNode->bytes);
        return buffer;
    }

    /**
     * 写文件
     * @param fp 打开文件的文件指针
     * @param buffer 文件缓冲区
     * @return 操作成功与否
     */
    bool writeFile(FilePointer fp, Buffer buffer) {
        if (!(fp.type & (fio::out| fio::app))) return false;
        if (fp.type & fio::app) {
            size_t size = buffer.size;
            if (fp.iNode->bytes != 0) {
                size_t headSize;
                bid_t * blockID = getBlockID(* fp.iNode, fp.iNode->blocks - 1);
                char block[_BLOCK_SIZE];
                _vhdc.readBlock(block, * blockID);
                headSize = std::min(buffer.size, size_t (_BLOCK_SIZE - fp.iNode->bytes));
                BufferTool().copy(block + fp.iNode->bytes, buffer.data, headSize);
                _vhdc.writeBlock(block, *blockID);
                size -= headSize;
            }
            if (size != 0) {
                bid_t blockID;
                char *p = buffer.data;
                for (int i = 0; i < size / _BLOCK_SIZE; i++) {
                    _fbc.distribute(blockID);
                    _vhdc.writeBlock(p, blockID);
                    push(*fp.iNode, blockID);
                    p += _BLOCK_SIZE;
                }
                _fbc.distribute(blockID);
                push(*fp.iNode, blockID);
                fp.iNode->bytes = static_cast<unsigned int>(size % _BLOCK_SIZE);
                _vhdc.writeBlock(p, blockID);
            }
        } else if (fp.type & fio::out) {
            for (int i = 0; i < fp.iNode->blocks; ++ i) {
                bid_t * blockID = pop(*fp.iNode);
                _fbc.recycle(* blockID);
            }
            fp.iNode->size = 0;
            fp.iNode->bytes = 0;
            fp.type |= fio::app;
            writeFile(fp, buffer);
        }
        _vhdc.writeBlock((char *) fp.iNode, fp.iNode->bid);
        return true;
    }
    /**
     * 移动文件
     * @param curINode
     * @param oldPath
     * @param newPath
     * @param fileName
     * @return
     */

    // TODO: test moveFile
    bool moveFile(INode * oldPath, INode * newPath, string fileName) {
//        if (!accessible(*newPath, mode::write) ||
//            !accessible(*oldPath, mode::write)) return false;
        INode * iNode = getINode(oldPath, fileName);
        addDirChild(*newPath,*iNode);
        removeDirChild(*oldPath,*iNode);
        return true;
    }
    /**
     * 复制文件到指定文件夹
     * @return
     */
    bool copyFile(INode * oldPath, INode * newPath, string fileName) {
//        if (!accessible(*oldPath, mode::read) ||
//            !accessible(*newPath, mode::write)) return false;
        INode * iNode = getINode(oldPath, std::move(fileName));
        if (!iNode) return false;
        char block[_BLOCK_SIZE];
        INode * iNodeNew = newINode();
        for (int i = 0; i < iNode->blocks; ++ i) {
            bid_t * blockID = getBlockID(*iNode, i);
            _vhdc.readBlock(block, *blockID);
            _fbc.distribute(*blockID);
            _vhdc.writeBlock(block, *blockID);
            push(*iNodeNew, *blockID);
        }
        addDirChild(*newPath, *iNodeNew);
        bid_t blockID;
        _fbc.distribute(blockID);
        iNodeNew->bytes = iNode->bytes;
        iNodeNew->size = iNode->size;
        iNodeNew->bid = blockID;
        iNode->mtime = iNode->ctime = time(nullptr);
        _vhdc.writeBlock((char *)iNodeNew, blockID);
        return true;
    }
};


#endif // EFS_FSC_H
