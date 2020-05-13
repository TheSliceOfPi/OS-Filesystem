#ifndef _HOLLYFS_H_
#define _HOLLYFS_H_

#define HOLLYFS_MAGIC 0xDEADF00D
#define HOLLYFS_DEFAULT_BLOCKSIZE 4096
#define HOLLYFS_DEFAULT_INODE_TABLE_SIZE 1024
#define HOLLYFS_DEFAULT_DATA_BLOCK_TABLE_SIZE 1024
#define HOLLYFS_FILENAME_MAX 255
struct hollyfs_super_block{
	long long magic_num; //to determine the super block by its magic number
	long long block_size; // in order to know how much space our superblock will be taking up
	
	long long inode_table_size; //To determine how big the table holding the inode info will be
	long long inode_count;//the number of inodes we will be keeping track of
	
	long long data_block_table_size; //to determine how big the datablock table  holding the datablock info will be
	long long data_block_count; //the number of data blocks
	};
static const long long HOLLYFS_SUPERBLOCK_BLOCK_NO=0; //Which block is the sb located, I set it to the front since its important(first thing read)
static const long long HOLLYFS_INODE_TABLE_START_BLOCK_NO=1; 
static const long long HOLLYFS_ROOTDIR_INODE_NO=2;//The datablock im placing the root dir into
static const long long HOLLYFS_ROOTDIR_DATA_BLOCK_NO_OFFSET=0;

struct hollyfs_inode{
	mode_t mode;
	long long inode_num; //So we know which node we are currently looking at
	long long data_block_no; //So we know which node this corresponds to
	union {
		long long  child_count; //if dir we keep track of how many things in the dentry
		long long file_size; // if file then we keep track of how big the file is
	};
};
struct hollyfs_dentry{
	char filename[HOLLYFS_FILENAME_MAX]; //A file name cannot be longer than 255 characters (2^8-1)
	long long inode_no; // We need to know which node we have to follow in order to find the right data blocks
};

static long long  HOLLYFS_INODES_PER_BLOCK_HSB(struct hollyfs_super_block *hollyfs_sb){ //# inodes per datablock
	return hollyfs_sb->block_size /sizeof(struct hollyfs_inode);
	}
static long long HOLLYFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(struct hollyfs_super_block  *hollyfs_sb){ //which datablock we are seeing in the table
	return HOLLYFS_INODE_TABLE_START_BLOCK_NO +hollyfs_sb->inode_table_size /HOLLYFS_INODES_PER_BLOCK_HSB(hollyfs_sb)+1;
	}

#endif
