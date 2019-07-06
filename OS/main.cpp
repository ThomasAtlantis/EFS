#include "os.h"

int main(int argc, char * argv[]) {
    #if DEBUG
    vector<string> diskLocations {
        "SLOT_1/data.vhd",
        "SLOT_2/data.vhd",
        "SLOT_X/data.vhd"
    };
    bid_t sliceStart = _OS_PART_BLOCK_SIZE;
    #else
    vector<string> diskLocations(3);
    bid_t sliceNum = _OS_PART_BLOCK_SIZE;
    for (int i = 1; i < argc; ++ i) {
        if (!strcmp(argv[i], "-m")) diskLocations[0] = argv[i + 1];
        else if (!strcmp(argv[i], "-v")) diskLocations[1] = argv[i + 1];
        else if (!strcmp(argv[i], "-p")) diskLocations[2] = argv[i + 1];
        else if (!strcmp(argv[i], "-i")) sliceNum = static_cast<bid_t>(std::stol(string(argv[i + 1])));
    }
    #endif
    OS os(diskLocations, sliceStart, _SLICE_BLOCK_SIZE);
//    从文件加载参数Demo
//    char x[] = "x = 10\ny = 100\nk = 3.44\nm =  abc\n";
//    for (const auto &record: stringTool().split(x, sizeof(x), '\n')) {
//        for (auto &value: stringTool().split(record, "=")) {
//            cout << stringTool().strip(value) << " ";
//        }
//        cout << endl;
//    }
    return 0;
}