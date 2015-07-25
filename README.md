FUSE ( File System in User Space )
====

This is an implementation of a file system in memory . Files are added, modified and deleted in memory. The default calls to the OS like read(), write(), truncate(), etc. are over-ridden by the custom APIs in this program. Coding is in C++.

USAGE: ./ramdisk <mount point dir> <size of ramdisk(MB)>
example : ./ramdisk temp 500

where temp is a directory under the current working directory

The RAMDISK.htm file has the description of the project requirements
The ramdisk.cpp file is the main program file
The ramdisk.h is the header file
Makefile is needed to be used to compile the code and create the executable
