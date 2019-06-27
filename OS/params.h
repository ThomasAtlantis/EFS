//
// Created by MAC on 2019/6/19.
//

#ifndef EFS_UTILITY_H
#define EFS_UTILITY_H

// VHD和FBC层
#define _BLOCK_SIZE 1024
#define _OS_PART_BLOCK_SIZE 4096
#define _BLOCK_GROUP_SIZE 100

// FSC和VFS层
#define _USER_FILE_NAME ......
#define _FILENAME_MAXLEN 20
#define _USERNAME_MAXLEN 20
// 需保证_BLOCK_INDEX_SIZE > _3INDEX_NUM + _2INDEX_NUM
#define _BLOCK_INDEX_SIZE 10
#define _3INDEX_NUM 1
#define _2INDEX_NUM 2
#define _1INDEX_NUM (_BLOCK_INDEX_SIZE \
    - _3INDEX_NUM \
    - _2INDEX_NUM)

#endif //EFS_UTILITY_H
