### 仿真文件系统 （Emulative File System）
注意：
+ README大家可以随时改写，但要单独提交（保证改写这个文件的同时没有修改其他文件，并注明修改的内容提纲）
+ 接口文档请另写一个markdown文件
+ 大家多交流，接口勤对
#### 0 分工表
|姓名|任务|
|----|----|
|刘尚育|VHD、CTL|
|王国秋|FS、VFS|
|胡彬梅|APP|
#### 1 整体思想
+ 系统共分为如下四个层次：
    + 虚拟硬盘层（VHD）：模拟硬盘的按块读写
    + 控制层（CTL）：空闲块管理
    + 虚拟文件系统层（VFS）：文件系统的功能实现；VFS解析文件路径，分发命令到不同分区对应的文件系统
    + 应用层（APP）：与用户交互的命令行界面，文件系统的功能调用
+ 文件系统启动流程：
    + 开机运行BIOS（用程序模拟），硬件自检，主要检查是否挂载了主硬盘
    + -> 使用BIOS自带的轻量级文件系统扫描主硬盘中是否包含操作系统的img文件
    + -> 若不包含，从移动硬盘安装系统，完成分区的初始化
    + -> 如果有操作系统，解压并运行操作系统程序，管理权转交给操作系统（BIOS程序退出）
    + -> 操作系统初始化
    + -> 运行命令行程序
    + -> 提示用户登录
    + -> 进入命令输入界面
    + -> 用户退出系统
    + -> 保存缓冲到硬盘
    + -> 关机
    + 其中读写文件借用内存缓冲区，修改文件后在用户保存时刷到硬盘上
+ 工程文件目录结构：
    + OS：我们编写的操作系统（文件系统），不同层次的代码在对应的文件夹下
        + main.cpp：主调程序，不要写其他代码
        + params.h：公用参数文件，公用的和系统相关的宏定义
    + VirtualMachine：运行程序的虚拟环境
        + SLOT_1 ~ SLOT_3： 硬盘插槽，其中SLOT_1为主硬盘插槽，每个插槽中是一个二进制文件
        + SLOT_X：移动硬盘总线，包含若干USB口，每个USB下也是一个二进制文件
    + VM：虚拟机相关代码，比如BIOS
    + CMakeLists.txt：CLion编译用的工程文件，指定了编译参数和路径
    + utilities.h：公用工具函数和头文件
+ 尽量少地使用Windows系统调用，避免使用system()函数，除非是在模拟层间接口
+ 命令行为多个命令可执行文件的集合，可裁剪，可替换
#### 2 技术关键及示例
+ 调exe的方法可参考bios.h中的
``` C++
HINSTANCE hNewExe = ShellExecute(NULL, "open", OSLocation.c_str(), NULL, NULL, SW_SHOW);
```
+ 解析main参数的示例：
``` C++
try { // 解析命令行参数
        for (int i = 1; i < argc; ++ i) {
            if (strcmp(argv[i], "--seed") == 0) _rand.seed(atoi(argv[i + 1])); // 随机数种子
            else if (strcmp(argv[i], "--frame_total") == 0) processFrameTotal = atoi(argv[i + 1]); // 进程分配页面总数
        }
    } catch (int index) {
        cout << "wrong parameter form!" << endl;
        return 0;
    }
```
#### 3 拓展点
+ 命令行tab补全与命令历史
+ 命令行vim编辑器
+ 文件系统搜索优化：哈希索引，B+树索引
#### 4 时间线
+ 周二晚要给出明确的接口文档