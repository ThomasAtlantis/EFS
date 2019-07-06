//
// Created by MAC on 2019/7/6.
//

#ifndef EFS_PART_H
#define EFS_PART_H

#include "../../utilities.h"
#include "../params.h"

#define PART_DATA_PADDING (_BLOCK_SIZE \
    - sizeof(int) \
    - sizeof(bid_t) * (_MAX_PARTITIONS * 3 + 1) \
    - _MAX_PARTITIONS * _FILENAME_MAXLEN)

typedef struct {
    int diskCount; // 硬盘数量
    bid_t partCount; // 分区数量
    bid_t partSizes[_MAX_PARTITIONS]; // 各分区大小
    bid_t partStart[_MAX_PARTITIONS]; // 各分区起始
    bid_t partDiskN[_MAX_PARTITIONS]; // 各分区硬盘
    char partNames[_MAX_PARTITIONS][_FILENAME_MAXLEN]; // 各分区名
    char padding[PART_DATA_PADDING];
} PartData;

#endif //EFS_PART_H
