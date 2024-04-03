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


int my_symlink(char *pathname) {
  //printf("\nmy_symlink Not yet implemented\n");
  
  //Get the new filename
  char newFilename[128];
  printf("input NEW_filename: ");
  fgets(newFilename, 128, stdin);
  newFilename[strlen(newFilename) - 1] = 0;

  //1.
  int oino = getino(pathname);
  MINODE *omip = iget(dev, oino);
  int mode = omip->INODE.i_mode;

  if (getino(newFilename) != 0) {
    printf("LINK Error: File must not already exist, %s \n", newFilename);
    return -1;
  }

  //2.
  my_creat(newFilename);
  int ino = getino(newFilename);
  MINODE *mip = iget(dev,ino);
  mip->INODE.i_mode =  0xA000;

  //3.
  char oldFilename[60];
  strcpy(oldFilename, pathname);
  strcpy((char*)mip->INODE.i_block, oldFilename);
  mip->INODE.i_size = strlen(pathname);
  mip->dirty = 1;
  iput(mip);

  omip->dirty = 1;
  iput(omip);

  return -1;
}

int my_readlink(char *pathname) {
  printf("\nmy_readlink Not yet implemented\n");
  return -1;
}

int readlink_h(char *filename, char *buffer) {
  int pino = getino(filename);

  if (pino == 0) {
    printf("Readlink Error: pino == 0 for file: %s \n", filename);
  }

  MINODE *mip = iget(dev, pino);
  int mode = mip->INODE.i_mode;

  if (!S_ISLNK(mode)) {
    printf("Readlink Error: !S_ISLNK for file: %s \n", filename);
    return -1;
  }

  strcpy(buffer, mip->INODE.i_block);

  return mip->INODE.i_size;
}