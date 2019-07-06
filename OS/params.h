//
// Created by MAC on 2019/6/19.
//

#ifndef EFS_UTILITY_H
#define EFS_UTILITY_H

typedef size_t bid_t; // u_int64(unsigned long long int)

// VHD和FBC层
#define _BLOCK_SIZE 1024
#define _OS_PART_BLOCK_SIZE 4096
#define _BLOCK_GROUP_SIZE 100

// FSC和VFS层
#define _USER_FILE_NAME ......
#define _FILENAME_MAXLEN 20
#define _USERNAME_MAXLEN 20
#define _DISKNAME_MAXLEN 20
// 需保证_BLOCK_INDEX_SIZE > _3INDEX_NUM + _2INDEX_NUM
#define _BLOCK_INDEX_SIZE 10
#define _3INDEX_NUM 1
#define _2INDEX_NUM 2
#define _1INDEX_NUM (_BLOCK_INDEX_SIZE \
    - _3INDEX_NUM \
    - _2INDEX_NUM)

// OS
#define _MAX_USERS 16
#define _PARTSIZE_SYS 1024
#define _PARTNAME_INFO ("/info")
#define _PARTNAME_SYS ("/sys")
#define _PARTNAME_SHARE ("/share")
#define _PARTNAME_USER ("/usr")
#define _SUPERADMIN_NAME ("root")
#define _INSTALL_FLAG_OFFSET 0
#define _USER_DATA_OFFSET 1
#define _PART_DATA_OFFSET 2
#define _INFORMATION_SIZE 3
#define _SLICE_BLOCK_SIZE 4096
#define _MAX_PARTITIONS 10
#define _PASSWORD_LENGTH 6
#define _MAX_DISKS 4

#endif //EFS_UTILITY_H
