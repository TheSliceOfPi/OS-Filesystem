#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>

#include "mount.h"



int main(char * argc , char ** argv)
{	int fd;
	long long ret;
	long long readme_inode_no;
	long long readme_datablock_no_offset;
	
	fd= open(argv[1],O_RDWR); //We are opening the device, O_RDWR so that we can read and write into the device.
	if(fd==-1){
		perror("Error opening the device");
		return -1;
	}
	
	//Lets begin by creating the superblock
	struct hollyfs_super_block hollyfs_sb = {
		.magic_num=HOLLYFS_MAGIC,
		.block_size=HOLLYFS_DEFAULT_BLOCKSIZE,
		.inode_table_size=HOLLYFS_DEFAULT_INODE_TABLE_SIZE,
		.inode_count=2, //One for dir and one for reg
		.data_block_table_size=HOLLYFS_DEFAULT_DATA_BLOCK_TABLE_SIZE,
		.data_block_count=2, //each node has its datablock
		};
	
	//root folder's inode
	struct hollyfs_inode hollyfs_root_inode={
		.mode=S_IFDIR | 0777, //set as directory and 0777 so everyone can read,write, AND execute. 
		.inode_num=HOLLYFS_ROOTDIR_INODE_NO,
		.data_block_no=HOLLYFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&hollyfs_sb)+HOLLYFS_ROOTDIR_DATA_BLOCK_NO_OFFSET,
		.child_count=1, //the only child(item in dentry) is the redme.txt file
		};
	//create readme.txt file
	char readme_text[]="You are reading the readme.txt file! \n"; //this is what is written in the file
	readme_inode_no=HOLLYFS_ROOTDIR_INODE_NO+1;
	readme_datablock_no_offset=HOLLYFS_ROOTDIR_DATA_BLOCK_NO_OFFSET+1;
	
	//readme.txt inode
	struct hollyfs_inode hollyfs_readme_inode={
		.mode=S_IFREG | 0777, //set as file and 0777 so everyone can read,write, AND execute
		.inode_num=readme_inode_no,
		.data_block_no=HOLLYFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&hollyfs_sb)+readme_datablock_no_offset,
		.file_size=sizeof(readme_text), //the size of the file is the size of the text that will be written in.
		};
		
	//Dentry table in the root folder
	struct hollyfs_dentry root_dentry_table[]={
		{
			.filename="readme.txt", //the only file in the dentry
			.inode_no=readme_inode_no, //we need to keep track of which inode to look at to read the file
		},
		};
	ret=0;
	do{ //since write returns the size that was written then it has to be the same size as the item itself. Otherwise it isnt writing properly.
		//l_seek will repositionread/write.
		//write super block
		if(sizeof(hollyfs_sb)!=write(fd,&hollyfs_sb,sizeof(hollyfs_sb))){//since write returns the amount that was written the size of holly_sb should be the amount that is written
			ret=-1;
			break;
			}
		if((off_t)-1 ==lseek(fd,hollyfs_sb.block_size,SEEK_SET)){
			ret=-2;
			break;
			}
		//write root inode
		if(sizeof(hollyfs_root_inode)!=write(fd,&hollyfs_root_inode,sizeof(hollyfs_root_inode))){
			ret=-5;
			break;
		}
		//write readme.txt inode
		if(sizeof(hollyfs_readme_inode)!=write(fd,&hollyfs_readme_inode,sizeof(hollyfs_readme_inode))){
			ret=-6;
			break;
		}
		//write root inode datablock
		if((off_t)-1 ==lseek(fd,HOLLYFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&hollyfs_sb)*hollyfs_sb.block_size,SEEK_SET)){
			ret=-7;
			break;
		}
		if(sizeof(root_dentry_table)!=write(fd,root_dentry_table,sizeof(root_dentry_table))){
			ret=-8;
			break;
		}
		if((off_t)-1 ==lseek(fd,(HOLLYFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&hollyfs_sb)+1)*hollyfs_sb.block_size,SEEK_SET)){
			ret=-9;
			break;
		}
		if(sizeof(readme_text)!=write(fd,readme_text, sizeof(readme_text))){
			ret=-10;
			break;
		}
	}while(0);
	close(fd);
	return ret; //must return 0, anything else is an error during writing or seeking
}
