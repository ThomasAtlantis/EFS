//
// Created by MAC on 2019/6/26.
//

#include "format_tool.h"

int main() {
    FormatTool formatTool("data.vhd");
    formatTool.compress("OS.exe");
    formatTool.decompress("OS.iso");
    return 0;
}