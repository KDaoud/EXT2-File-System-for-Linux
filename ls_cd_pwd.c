#include "type.h"

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev;
//extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];

/************* cd_ls_pwd.c file **************/

int my_cd(char *pathname)
{
  if (pathname[0] == 0) {
    strcpy(pathname, "../");
  }

  int inode = getino(pathname);
  
  if (inode == 0) {
    printf("INODE not found for path: %s \n", pathname);
    return 1;
  }

  MINODE *mip = iget(dev, inode);

  if (!S_ISDIR(mip->INODE.i_mode)) {
    printf("MIP is not a valid directory for path: %s \n", pathname);
    return 1;
  }

  iput(running->cwd);
  running->cwd = mip;

  return 0;
}

int ls_file(MINODE *mip, char *name)
{
  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";

  char ftime[64];

  if ((mip->INODE.i_mode & 0xF000) == 0x8000)
    printf("%c", '-');
  if ((mip->INODE.i_mode & 0xF000) == 0x4000)
    printf("%c", 'd');
  if ((mip->INODE.i_mode & 0xF000) == 0xA000)
    printf("%c", 'l');
  for (int i=8; i >= 0; i--) {
    if (mip->INODE.i_mode & (1 << i))
      printf("%c", t1[i]);
    else
      printf("%c", t2[i]);
  }

  printf("%4d ", mip->INODE.i_links_count);
  printf("%4d ", mip->INODE.i_gid);
  printf("%4d ", mip->INODE.i_uid);
  
  time_t time = mip->INODE.i_ctime;
  strcpy(ftime, ctime(&time));
  ftime[strlen(ftime)-1] = 0;
  
  printf("%s ",ftime);
  printf("%8d ", mip->INODE.i_size);
  printf("%11s", basename(name));

  printf(" [%d %2d]", mip->dev, mip->ino);
  
  if ((mip->INODE.i_mode & 0xF000)== 0xA000){
    char buffer[512];
    int size = readlink_h(name, buffer);
    printf(" -> %s", buffer);
  }

  printf("\n");
}

int ls_dir(MINODE *mip)
{
  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;
  
  while (cp < buf + BLKSIZE) {
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;

    MINODE *tempMip = iget(dev, dp->inode);
    ls_file(tempMip, temp);

    //printf("%s  ", temp);

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }

  printf("\n");
}

int my_ls(char *pathname)
{
  //printf("ls: list CWD only!");
  if (pathname[0] != 0) {
    int *useDev = &dev;
    int ino = getino_dev(pathname, useDev);
    if (ino == 0) {
      printf("INO not found for path: %s \n", pathname);
      return -1;
    }

    MINODE *mip = iget(*useDev, ino);

    if (S_ISDIR(mip->INODE.i_mode)) {
      ls_dir(mip);
    }
    else {
      char *dirName = name[n - 1];
      ls_file(mip, dirName);
    }
  }
  else {
    ls_dir(running->cwd);  
  }
}

char *rpwd(MINODE *wd)
{
  if (wd == root) {
    return;
  }
  
  int my_ino;
  int *dev = &wd->dev;
  int parent_ino = findino(wd, &my_ino, dev);

  MINODE *pip = iget(*dev, parent_ino);

  char name[128];
  findmyname(pip, my_ino, name);

  for (int i = 0; i < 128; i++) {
    if (name[i] == '\r') {
      name[i] = '\0';
      break;
    }
    if (name[i] == '\n') {
      name[i] = '\0';
      break;
    } 
    if (name[i] == '\0') {
      break;
    }
  }

  rpwd(pip);

  printf("/%s", name);
}


void my_pwd(MINODE *wd)
{
  if (wd == root){
    printf("/\n");
    return;
  }
  
  rpwd(wd);
  printf("\n");
}
