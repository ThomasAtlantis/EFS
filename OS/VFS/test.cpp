//
// Created by 12203 on 2019/6/26.
//
#include "../FSC/fsc.h"

void pushTest() {
    FSController fsc("./SLOT_2/data.vhd", 0, 1000, "/home");
    fsc._fbc.formatting();
    auto * iNode = fsc.newINode();
    bid_t blockID;
    for (bid_t i = 0; i < 7; ++ i) {
        fsc._fbc.distribute(blockID);
        fsc.push(* iNode, blockID);
    }
    fsc._fbc.distribute(blockID); // 7 + 1 分配2级索引块
    fsc.push(* iNode, blockID);
    for (bid_t i = 0; i < 127; ++ i) {
        fsc._fbc.distribute(blockID);
        fsc.push(* iNode, blockID);
    }
    fsc._fbc.distribute(blockID); // 7 + 128 + 1 分配2级索引块
    fsc.push(* iNode, blockID);
    for (bid_t i = 0; i < 127; ++ i) {
        fsc._fbc.distribute(blockID);
        fsc.push(* iNode, blockID);
    }
    fsc._fbc.distribute(blockID); // 7 + 128 * 2 + 1 连续分配3级两个索引块
    fsc.push(* iNode, blockID);
    for (bid_t i = 0; i < 127; ++ i) {
        fsc._fbc.distribute(blockID);
        fsc.push(* iNode, blockID);
    }
    fsc._fbc.distribute(blockID); // 7 + 128 * 2 + 128 + 1 分配3级索引块
    fsc.push(* iNode, blockID);
    for (bid_t i = 0; i < 127; ++ i) {
        fsc._fbc.distribute(blockID);
        fsc.push(* iNode, blockID);
    }
    fsc._fbc.distribute(blockID); // 7 + 128 * 2 + 128 * 2 + 1 分配3级索引块
    fsc.push(* iNode, blockID);
}

void getBlockIDTest() {
    FSController fsc("./SLOT_2/data.vhd", 0, 1000, "/home");
    fsc._fbc.formatting();
    auto * iNode = fsc.newINode();
    bid_t blockID;
    bid_t * tmp;
    for (bid_t i = 0; i < 7; ++ i) {
        fsc._fbc.distribute(blockID);
        fsc.push(* iNode, blockID);
    }
    fsc._fbc.distribute(blockID); // 7 + 1 分配2级索引块
    fsc.push(* iNode, blockID);
    tmp = fsc.getBlockID(* iNode, 7);
    for (bid_t i = 0; i < 127; ++ i) {
        fsc._fbc.distribute(blockID);
        fsc.push(* iNode, blockID);
    }
    fsc._fbc.distribute(blockID); // 7 + 128 + 1 分配2级索引块
    fsc.push(* iNode, blockID);
    tmp = fsc.getBlockID(* iNode, 7 + 128);
    for (bid_t i = 0; i < 127; ++ i) {
        fsc._fbc.distribute(blockID);
        fsc.push(* iNode, blockID);
    }
    fsc._fbc.distribute(blockID); // 7 + 128 * 2 + 1 连续分配3级两个索引块
    fsc.push(* iNode, blockID);
    tmp = fsc.getBlockID(* iNode, 7 + 128 * 2);
    for (bid_t i = 0; i < 127; ++ i) {
        fsc._fbc.distribute(blockID);
        fsc.push(* iNode, blockID);
    }
    fsc._fbc.distribute(blockID); // 7 + 128 * 2 + 128 + 1 分配3级索引块
    fsc.push(* iNode, blockID);
    tmp = fsc.getBlockID(* iNode, 7 + 128 * 3);
    for (bid_t i = 0; i < 127; ++ i) {
        fsc._fbc.distribute(blockID);
        fsc.push(* iNode, blockID);
    }
    fsc._fbc.distribute(blockID); // 7 + 128 * 2 + 128 * 2 + 1 分配3级索引块
    fsc.push(* iNode, blockID);
    tmp = fsc.getBlockID(* iNode, 7 + 128 * 4);
    int k = 0;
}

int main() {
    getBlockIDTest();
    return 0;
}
