all: ramdisk

ramdisk: ramdisk.cpp ramdisk.h 
	g++ -w ramdisk.cpp -o ramdisk `pkg-config fuse --cflags --libs` -Wall -Werror

clean:
	rm -rf  ramdisk.o ramdisk
