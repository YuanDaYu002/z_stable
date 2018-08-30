编译 新平台 海思ipc: make
编译 新平台 安霸ipc：make -f ambaMakefile
编译 新平台 vatics： make -f vaticsMakefile
编译 新平台 NVR：    make -f nwsMakefile
编译 旧平台 海思ipc: make -f Makefile.sip
编译 旧平台 NVR:     make -f nvrMakefile

编译海思的项目时，将src文件夹放到源代码根目录下再编译；
编译安霸和vatics时，将src文件夹放到zmdnetlib目录下再编译;
