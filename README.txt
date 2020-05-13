Edith Flores
OS Filesystem

Files:
hollyfs.c
Makefile
mkfs.c
mount.c


--------hollyfs-----------------
hollyfs is a  simple,  read-only  filesystem  as  a  linux  kernel  module. The kernel  module can  perform  the following tasks:
-Instantiate itself as a kernel module which can be inserted (insmod) and removed (rmmod).
-Register a valid filesystem with the rest of the linux kernel.
-Support “mounting” and “umounting’ of the filesystem.
-Read the superblock from the device (microSD card partition).
      •Read the root folder from the device.
      •Report the superblock and root folder to the rest of the linux kernel.
-Implement iteration and lookup on directories.
-Implement reading of files.

To run:
