/***** type.h file for EXT2 FS *****/

#ifndef TYPE
#define TYPE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>


typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// define shorter TYPES for convenience
typedef struct ext2_group_desc GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode INODE;
typedef struct ext2_dir_entry_2 DIR;
#define BLKSIZE 1024

// Block number of EXT2 FS on FD
#define SUPERBLOCK 1
#define GDBLOCK 2
#define ROOT_INODE 2

// Default dir and regular file modes
#define DIR_MODE 0x41ED
#define FILE_MODE 0x81AE
#define SUPER_MAGIC 0xEF53
#define SUPER_USER 0

// Proc status
#define FREE 0
#define BUSY 1
#define READY 1

// file system table sizes
#define NMINODE 100
#define NMTABLE 10
#define NPROC 2
#define NFD 10
#define NOFT 40

// Open File Table
typedef struct oft{
  int mode;
  int refCount;
  struct minode *minodePtr;
  int offset;
}OFT;

// PROC structure
typedef struct proc{
  struct Proc *next;
  int pid;
  int uid;
  int gid;
  int ppid;
  int status;
  struct minode *cwd;
  OFT *fd[NFD];
}PROC;

// In-memory inodes structure
typedef struct minode{
  INODE INODE; // disk inode
  int dev, ino;
  int refCount; // use count
  int dirty; // modified flag
  int mounted; // mounted flag
  struct MTABLE *mntPtr; // mount table pointer
  // int lock; // ignored for simple FS
}MINODE;

// Mount Table structure
typedef struct mtable{
  int dev; // device number; 0 for FREE
  int ninodes; // from superblock
  int nblocks;
  int free_blocks; // from superblock and GD
  int free_inodes;
  int bmap; // from group descriptor
  int imap;
  int iblock; // inodes start block
  MINODE *mntDirPtr; // mount point DIR pointer
  char devName[64]; //device name
  char mntName[64]; // mount point DIR name
}MTABLE;


/* util.c */
int get_block(int dev, int blk, char *buf);

int put_block(int dev, int blk, char *buf);

int tokenize(char *pathname);

MINODE *iget(int dev, int ino);

void iput(MINODE *mip);

int getino_dev(char *pathname, int *dev);

int getino(char *pathname);

int search(MINODE *mip, char *name);

int findmyname(MINODE *parent, u32 myino, char myname[]);

int findino(MINODE *mip, u32 *myino, int *dev);

int is_empty(MINODE *mip);

//mountroot.c
int fs_init();
int mount_root(char *rootdev);

/* cd_ls_pwd.c */
int my_cd(char *pathname);

int ls_file(MINODE *mip, char *name);

int ls_dir(MINODE *mip);

int my_ls(char *pathanme);

void my_pwd(MINODE *wd);

char *rpwd(MINODE *wd);

int my_mkdir(char *pathname);

int my_creat(char *pathname);

int my_rmdir(char *pathname);

int my_link(char *pathname);

int my_unlink(char *pathname);

int my_symlink(char *pathname);

int my_readlink(char *pathname);
int readlink_h(char *filename, char *buffer);

int my_stat(char *pathname);

int my_chmod(char *pathname);

int my_utime(char *pathname);

int tst_bit(char *buf, int bit);

int set_bit(char *buf, int bit);

int decFreeInodes(int dev);

int balloc(int dev);

MTABLE *get_mtable(int dev);

int ialloc(int dev);

int enter_name(MINODE *pip, int ino, char *name);

//rmdir.c
int rm_child(MINODE *pmip, char *name);

MINODE *mialloc();

int bdalloc(int dev, int blk);

int idalloc(int dev, int ino);

int get_path_and_file(char *input, char *filename, char *filepath);

int truncate(MINODE *mip);

//read_cat.c
int map(MINODE *mip, int lbk);
int myread(int fd, char *buf, int n_bytes);
int read_file();
int my_cat(char *pathname);

//open_close_lseek.c
int my_open(char *filename, int flags);
int open_file(char *filename);
int my_close_file(int fd);


//write.c
int my_write(int fd, char buf[ ], int nbytes);

//cp.c
int my_cp(char *filepath);

//mount_unmount.c

int my_mount(char* filepath);
int my_unmount(char* filepath);

#endif