进入例程

cd sample

进行编译，编译后的应用程序在相应文件夹（此处的交叉编译器可以使用arm-linux-gnueabihf-）

make -j12 ARCH=arm CROSS_COMPILE=arm-hisiv510-linux- 
