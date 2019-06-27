//
// Created by 12203 on 2019/6/26.
//
#include "../FSC/fsc.h"

void test_wgq()
{
    FSController fsc("./SLOT_2/data.vhd", 0, 1000, "/home");

    fsc.createDir();
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
    for (bid_t i = 0; i < 127; ++ i) { // freeCount == 700 崩溃
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



int main() {
    /*
    VHDController vhdc("./SLOT_2/data.vhd");
    FBController fbc(vhdc, 0, 1000);
    fbc.formatting();
    bid_t blockID;
    auto * ib = new FSController::IndexBlock;
    for (bid_t i = 0; i < _INDEXBLOCK_ITEM_SIZE; i ++)
        ib->itemList[i] = i;
    vhdc.writeBlock((char *)ib, 700);
    delete(ib);
    ib = new FSController::IndexBlock;
    vhdc.readBlock((char *)ib, 700);
    int i = 0;
//    for (int i = 0; i < 10; ++ i) {
//        for (int j = 0; j < 100; ++ j)
//            fbc.distribute(blockID);
//    }
    */
//    /*
/*
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
    for (bid_t i = 0; i < 127; ++ i) { // freeCount == 700 崩溃
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
    fsc.push(* iNode, blockID);*/
//     */
    return 0;
}
