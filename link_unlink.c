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


int my_link(char *pathname) {
  
  //Get the new filename
  char newFilename[128];
  printf("input NEW_filename: ");
  fgets(newFilename, 128, stdin);
  newFilename[strlen(newFilename) - 1] = 0;

  //1.
  int oino = getino(pathname);
  MINODE *omip = iget(dev, oino);
  int mode = omip->INODE.i_mode;

  if (S_ISDIR(mode)) {
    printf("Link Error: Must not be directory: %s", pathname);
    return -1;
  }

  //2.
  if (getino(newFilename) != 0) {
    printf("LINK Error: File must not already exist, %s \n", newFilename);
    return -1;
  }

  //3.
  char parent[128];
  char child[128];
  get_path_and_file(newFilename, &child, &parent);
  int pino = getino(parent);
  MINODE *pmip = iget(dev, pino);
  enter_name(pmip, oino, child);

  //4.
  omip->INODE.i_links_count++;
  omip->dirty = 1;
  iput(omip);
  iput(pmip);

  return 0;
}

int my_unlink(char *pathname) {

  //1.
  int ino = getino(pathname);
  MINODE *mip = iget(dev, ino);
  int mode = mip->INODE.i_mode;
  if (S_ISDIR(mode)) {
    printf("Unlink error: is DIR: %s \n", pathname);
    return -1;
  }

  //2.
  char *parent = dirname(pathname);
  char *child = basename(pathname);

  int pino = getino(parent);
  MINODE *pmip = iget(dev, pino);

  rm_child(pmip, child);
  pmip->dirty = 1;
  iput(pmip);

  //3.
  mip->INODE.i_links_count--;

  //4.
  if (mip->INODE.i_links_count > 0) {
    mip->dirty = 1;
  }
  else {
    for (int i = 0; i < 12; i++) {
      int blk = mip->INODE.i_block[i];
      if (blk == 0) {
        break;
      }
      bdalloc(dev, blk);
    }
    idalloc(mip->dev, mip->ino);
  }

  iput(mip);
  return 0;
}