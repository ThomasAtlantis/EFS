#include "os.h"

int main(int argc, char * argv[]) {
    vector<string> diskLocations(3);
    bid_t slice = 0;
    for (int i = 1; i < argc; ++ i) {
        if (!strcmp(argv[i], "-m")) diskLocations[0] = argv[i + 1];
        else if (!strcmp(argv[i], "-v")) diskLocations[1] = argv[i + 1];
        else if (!strcmp(argv[i], "-p")) diskLocations[2] = argv[i + 1];
        else if (!strcmp(argv[i], "-i")) slice = static_cast<bid_t>(std::stol(string(argv[i + 1])));
    }
    OS os(diskLocations, slice);
//    char x[] = "x = 10\ny = 100\nk = 3.44\nm =  abc\n";
//    for (const auto &record: stringTool().split(x, sizeof(x), '\n')) {
//        for (auto &value: stringTool().split(record, "=")) {
//            cout << stringTool().strip(value) << " ";
//        }
//        cout << endl;
//    }
    return 0;
}