//
// Created by MAC on 2019/7/6.
//

#ifndef EFS_USER_H
#define EFS_USER_H

#include "params.h"
#include "../utilities.h"

#define USER_DATA_PADDING (_BLOCK_SIZE \
    - sizeof(int) - sizeof(time_t) - _MAX_USERS * (_USERNAME_MAXLEN + _PASSWORD_LENGTH + 1))

#pragma pack(1)
typedef struct {

    // 用户信息表：sizeof(int) * 1 + _MAX_USERS * (_USERNAME_MAXLEN + _PASSWORD_LENGTH + 1)
    time_t loginTime; // 上次访问时间
    int userCount; // 用户数量
    char userNames[_MAX_USERS][_USERNAME_MAXLEN]; // 用户名
    char passwords[_MAX_USERS][_PASSWORD_LENGTH]; // 用户密码
    char userGroup[_MAX_USERS]; // 用户组信息

    // 填充
    char padding[USER_DATA_PADDING];
} UserData; // 系统参数
#pragma pack()

#endif //EFS_USER_H
