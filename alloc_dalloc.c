#include "type.h"

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev;
////extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];


int bdalloc(int dev, int blk) {

   int i;
   char buf[BLKSIZE];
   
   MTABLE *mp = (MTABLE *)get_mtable(dev);

   if (blk > mp->nblocks){ // niodes global
      printf("block %d out of range\n", blk);
      return;
   }

   //Get the superblock for the device number passed in
   get_block(dev, mp->bmap, buf);


   clr_bit(buf, blk-1);

   // write buf back
   put_block(dev, mp->bmap, buf);

   // update free inode count in SUPER and GD
   incFreeBlocks(dev);

   return 0;
}

int idalloc(int dev, int ino) {
   int i;
   char buf[BLKSIZE];
   MTABLE *mp = (MTABLE *)get_mtable(dev);
   if (ino > mp->ninodes){ // niodes global
      printf("inumber %d out of range\n", ino);
      return 0;
   }
   // get inode bitmap block
   get_block(dev, mp->imap, buf);
   clr_bit(buf, ino-1);
   // write buf back
   put_block(dev, mp->imap, buf);
   // update free inode count in SUPER and GD
   incFreeInodes(dev);
}

int ialloc(int dev) {
   int i;
   char buf[BLKSIZE];
   // use imap, ninodes in mount table of dev
   MTABLE *mp = (MTABLE *)get_mtable(dev);
   get_block(dev, mp->imap, buf);
   for (i=0; i<mp->ninodes; i++) {
      if (tst_bit(buf, i)==0) {
         set_bit(buf, i);
         put_block(dev, mp->imap, buf);
         // update free inode count in SUPER and GD
         decFreeInodes(dev);
         return (i+1);
      }
   }
   return 0; // out of FREE inodes
}

int balloc(int dev) {

   MTABLE *mp = (MTABLE *)get_mtable(dev);
   
   //Get the superblock for the device number passed in
   char buf[BLKSIZE];
   get_block(dev, mp->bmap, buf);

   //Iterate through the blocks
   for (int i = 0; i < mp->nblocks; i++) {
      if (tst_bit(buf, i) == 0) {
         set_bit(buf, i);
         put_block(dev, mp->bmap, buf);
         decFreeBlocks(dev);
         bzero(buf, BLKSIZE); // optional: clear buf[ ] to 0
         put_block(dev, i+1, buf);
         return (i + 1);
      }
   }

   return -1;
}