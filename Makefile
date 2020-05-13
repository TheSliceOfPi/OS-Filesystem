obj-m += hollyfs.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	gcc mkfs.c -g -o mkfs


do: all
	sudo ./mkfs /dev/mmcblk0p3
	sudo insmod hollyfs.ko
	sudo mount -t hollyfs /dev/mmcblk0p3 ../mount_pt/
	ls -lh ../mount_pt/
	cat ../mount_pt/readme.txt
mount:
	sudo ./mkfs /dev/mmcblk0p3
	sudo insmod hollyfs.ko
	sudo mount -t hollyfs /dev/mmcblk0p3 ../mount_pt/
undo:
	sudo umount /home/pi/Documents/Homework4/mount_pt/
	sudo rmmod hollyfs