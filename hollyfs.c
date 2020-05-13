#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/namei.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/version.h>
#include <linux/jbd2.h>
#include <linux/parser.h>
#include <linux/blkdev.h>
#include "mount.h"

MODULE_LICENSE("GPL");
static struct kmem_cache *hollyfs_inode_cachep;
static const struct inode_operations hollyfs_dir_inode_ops;
static const struct file_operations hollyfs_dir_ops;
static const struct file_operations hollyfs_file_ops;
struct hollyfs_inode *hollyfs_get_inode(struct super_block *sb,long long inode_no){
	struct hollyfs_super_block *hollyfs_sb=sb->s_fs_info;
	struct hollyfs_inode *hfs_inode=NULL;
	struct hollyfs_inode *inode_buffer=NULL;
	int i;
	struct buffer_head *bh;
	bh=sb_bread(sb,HOLLYFS_INODE_TABLE_START_BLOCK_NO);
	hfs_inode=(struct hollyfs_inode *)bh->b_data;
	for (i=0;i<hollyfs_sb->inode_count;i++){ //look through each inode until we find the one we are looking for
		if(hfs_inode->inode_num ==inode_no){
			inode_buffer=kmem_cache_alloc(hollyfs_inode_cachep,GFP_KERNEL);
			memcpy(inode_buffer,hfs_inode,sizeof(*inode_buffer)); //copy it over since the buffer will be released at the end
			break;
		}
		hfs_inode++;
	}	
	brelse(bh);
	return inode_buffer;
	}
static int hollyfs_iterate_dir(struct file *filp ,struct dir_context *ctx){
	loff_t pos;
	struct inode *inode;
	struct super_block *sb;
	struct buffer_head *bh;
	struct hollyfs_inode *hfs_inode;
	struct hollyfs_dentry *record;
	int i;
	pos=ctx->pos;//checking that it is the same user-space
	inode=file_inode(filp);
	sb=inode->i_sb;
	if(pos){
		return 0;
		}
	hfs_inode=inode->i_private;
	bh=sb_bread(sb,hfs_inode->data_block_no);
	record=(struct hollyfs_dentry *)bh->b_data;
	for(i=0;i<hfs_inode->child_count;i++){ //iterate through the dentry 
		dir_emit(ctx,record->filename,HOLLYFS_FILENAME_MAX,record->inode_no,DT_UNKNOWN);//reporting entry
		ctx->pos+=sizeof(struct hollyfs_dentry);
		pos+=sizeof(struct hollyfs_dentry);
		record++;
		}
	brelse(bh);
	return 0;
}
static struct inode *hollyfs_iget(struct super_block *sb,int ino){
	struct inode *inode;
	struct hollyfs_inode *hfs_inode;
	
	hfs_inode=hollyfs_get_inode(sb,ino);
	inode=new_inode(sb);
	inode->i_ino=ino;
	inode->i_sb=sb;
	inode->i_op=&hollyfs_dir_inode_ops; //lookup
	if(S_ISDIR(hfs_inode->mode)){
		inode->i_fop=&hollyfs_dir_ops; //iterate through dentry if it is a dir
		}
	else if(S_ISREG(hfs_inode->mode)){
		inode->i_fop=&hollyfs_file_ops;//read if it is a file
		}
	inode->i_private=hfs_inode;
	return inode;
}
struct dentry *hollyfs_lookup(struct inode *parent_inode, struct dentry *child_dentry, unsigned int flags){
	struct hollyfs_inode *parent=parent_inode->i_private;
	struct super_block *sb=parent_inode->i_sb;
	struct buffer_head *bh;
	struct hollyfs_dentry *record;
	int i;
	bh=sb_bread(sb,parent->data_block_no);
	record=(struct hollyfs_dentry *)bh->b_data;
	for(i=0;i<parent->child_count;i++){ //look through every dir child, in this case we only have readme.txt
		if(0==strcmp(record->filename,child_dentry->d_name.name)){ //once we actually find the proper file by its name
			struct inode *inode =hollyfs_iget(sb,record->inode_no);
			inode_init_owner(inode,parent_inode,S_IFREG | 0777);
			d_add(child_dentry,inode);
			return NULL;
			}
		record++;
		}
	return 0;
}
static int hollyfs_read(struct file * filp,char __user * buf, size_t len, loff_t * ppos){
	struct hollyfs_inode *inode =filp->f_path.dentry->d_inode->i_private;
	struct buffer_head *bh;
	
	char *buffer;
	int nbytes;
	
	if(*ppos>=inode->file_size){
		return 0;
		}
	bh=sb_bread(filp->f_path.dentry->d_inode->i_sb,inode->data_block_no);
	buffer=(char *)bh->b_data;
	nbytes=min((size_t) inode->file_size,len);
	if(copy_to_user(buf,buffer,nbytes)){ //make sure it is actually reading and copying to the cmd line when cat-ing
		printk(KERN_ERR "Could not reach the user space to read.\n");
		return 0;
		}
	brelse(bh);
	*ppos+=nbytes;
	return nbytes;
}
static const struct inode_operations hollyfs_dir_inode_ops={
	.lookup=hollyfs_lookup, //find a given dentry entry/file
};
static const struct file_operations hollyfs_dir_ops={
	.iterate=hollyfs_iterate_dir, //tell a user-space process what files are in this dir
	};
static const struct file_operations hollyfs_file_ops={
	.read=hollyfs_read, //read the file once we are able to reach it
	};
static const struct inode_operations hollyfs_file_inode_ops={
	.getattr=simple_getattr, //implemented in fs.h
	};
/*void hollyfs_put_super(struct super_block *sb){
	return;
	}
const struct super_operations hollyfs_sb_ops={
	.put_super=hollyfs_put_super,
	};*/
static int hollyfs_fill_super(struct super_block *sb, void *data , int silent){
	struct inode *root_inode;
	struct buffer_head *bh;
	struct hollyfs_super_block *hollyfs_sb = NULL;
	int ret= 0; //if this is never altered then there is no error
	bh=sb_bread(sb,HOLLYFS_SUPERBLOCK_BLOCK_NO);
	hollyfs_sb=(struct hollyfs_super_block *)bh->b_data;
	
	 
	//setting our sb to the system's sb
	
	sb->s_magic=HOLLYFS_MAGIC; //give it the magic num
	sb->s_fs_info=hollyfs_sb; //sb info
	sb->s_maxbytes=HOLLYFS_DEFAULT_BLOCKSIZE; //blocksize of the sb
	
	//setting root's inode now
	root_inode=new_inode(sb);
	root_inode->i_ino=HOLLYFS_ROOTDIR_INODE_NO;
	inode_init_owner(root_inode,NULL,S_IFDIR | 0777);
	root_inode->i_sb=sb;
	root_inode->i_op=&hollyfs_dir_inode_ops;//struct to get to lookup
	root_inode->i_fop=&hollyfs_dir_ops;//struct to get to iterate
	root_inode->i_private=hollyfs_get_inode(sb,HOLLYFS_ROOTDIR_INODE_NO);
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0) //make root on newer kernels
	sb->s_root=d_make_root(root_inode);
#else 
	sb->s_root=d_alloc_root(root_inode);
	if(!sb->s_root){
		iput(root_inode);
		}
#endif
	brelse(bh); //release the buffer head
	return ret;
	}
	
static struct dentry *hollyfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void*data){
	struct dentry *ret;
	ret=mount_bdev(fs_type,flags,dev_name,data,hollyfs_fill_super); //preparing to mount, will lead to the sb fill function
	if(IS_ERR(ret)){
		printk(KERN_ERR "Error mounting hollyfs");
	}else{
		printk(KERN_INFO "hollyfs is succesfully mounted on [%s]\n",dev_name);
		}
	return ret;
}
static void hollyfs_kill_sb(struct super_block *sb){
	printk(KERN_INFO "simplefs superblock is destroyed.Unmount successful!!!.\n");
	kill_block_super(sb); //unmounts the fs
	return;
}
static struct file_system_type hollyfs_type={
	.name="hollyfs", //this is how it will appear when registered
	.owner = THIS_MODULE, //This as in I am the owner...
	.mount		=hollyfs_mount, //which function to head to when mounting
	.kill_sb	=hollyfs_kill_sb, //which function to head to when trying to unmount
	.fs_flags	=FS_USERNS_MOUNT,
	};
static int __init init_hollyfs_fs(void){
	int ret;
	hollyfs_inode_cachep=kmem_cache_create("hollyfs_inode_cache",sizeof(struct hollyfs_inode),0,(SLAB_RECLAIM_ACCOUNT|SLAB_MEM_SPREAD),NULL);//create cache to use
	ret=register_filesystem(&hollyfs_type); //registering the fs
	if(ret==0){
		printk(KERN_ALERT "Registered");//to print on dmesg when fs registered properly
		}
	return ret;
	}

static void __exit end_hollyfs_fs(void){
	int ret;
	printk(KERN_ALERT "Unregistered");
	ret=unregister_filesystem(&hollyfs_type); //unregister the fs
	kmem_cache_destroy(hollyfs_inode_cachep); //destroy any cache created
	
	
}
module_init(init_hollyfs_fs);
module_exit(end_hollyfs_fs);
