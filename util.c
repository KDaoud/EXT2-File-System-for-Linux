/*********** util.c file ****************/
#include "type.h"
#include <errno.h>

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MTABLE mtable[NMTABLE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev;
////extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   int n = read(dev, buf, BLKSIZE);
   if (n < 0) {
      printf("get_block [%d %d] error \n", dev, blk);
   }
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, SEEK_SET);
   int n = write(dev, buf, BLKSIZE);
   if (n != BLKSIZE) {
      printf("put_block [%d %d] error: %s \n", dev, blk, strerror(errno));
   }
}   

int tokenize(char *pathname)
{
  int i;
  char *s;
  //printf("tokenize %s\n", pathname);

  strcpy(gpath, pathname);   // tokens are in global gpath[ ]
  n = 0;

  s = strtok(gpath, "/");
  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }
  name[n] = 0;

   /*
  for (i= 0; i<n; i++)
    printf("%s  ", name[i]);
  printf("\n"); 
  */
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
   MINODE *mip;
   MTABLE *mp = get_mtable(dev);
   INODE *ip;
   int i, blk, offset;
   char buf[BLKSIZE];

   for (i=0; i<NMINODE; i++){
      mip = &minode[i];
      if (mip->refCount && mip->dev == dev && mip->ino == ino){
         mip->refCount++;
         //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
         return mip;
      }
   }

   mip = mialloc();
   mip->dev = dev;
   mip->ino = ino;
   blk = (ino - 1) / 8 + mp->iblock;
   offset = (ino - 1) % 8;
   get_block(dev, blk, buf);
   ip = (INODE*)buf + offset;
   mip->INODE = *ip;

   //Initialize MINODE
   mip->refCount = 1;
   mip->mounted = 0;
   mip->dirty = 0;
   mip->mntPtr = 0;

   return mip;
   
   /*
   for (i=0; i<NMINODE; i++){
      mip = &minode[i];
      if (mip->refCount == 0){
         //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
         mip->refCount = 1;
         mip->dev = dev;
         mip->ino = ino;

         // get INODE of ino to buf    
         blk    = (ino-1)/8 + iblk;
         offset = (ino-1) % 8;

         //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

         get_block(dev, blk, buf);
         ip = (INODE *)buf + offset;
         // copy INODE to mp->INODE
         mip->INODE = *ip;
      
         // initialize minode
         mip->refCount = 1;
         mip->mounted = 0;
         mip->dirty = 0;
         mip->mntPtr = 0;

         return mip;
      }
   }   
   */
   printf("PANIC: no more free minodes\n");
   return 0;
}

MINODE *mialloc() {
   int i;
   for (i = 0; i < NMINODE; i++) {
      MINODE *mp = &minode[i];
      if (mp->refCount == 0) {
         mp->refCount = 1;
         return mp;
      }
   }
   printf("FS panic: out of minodes \n");
   return 0;
}

int midalloc(MINODE *mip) {
   mip->refCount = 0;
}

void iput(MINODE *mip)
{

   MTABLE *mp = (MTABLE *)get_mtable(mip->dev);

   int i, block, offset;
   char buf[BLKSIZE];
   INODE *ip;

   if (mip==0) 
      return;

   mip->refCount--;
   
   if (mip->refCount > 0) return;
   if (!mip->dirty)       return;

   //Write INODE back to disk
   block = (mip->ino - 1) / 8 + mp->iblock;
   offset = (mip->ino - 1) % 8;

   //Get block containing this INODE
   get_block(mip->dev, block, buf);
   ip = (INODE*)buf + offset;
   *ip = mip->INODE;
   
   put_block(mip->dev, block, buf);

   midalloc(mip);
} 

int search(MINODE *mip, char *name)
{
   int i; 
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;

   //printf("search for %s in MINODE = [%d, %d]\n", name,mip->dev,mip->ino);
   ip = &(mip->INODE);

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(mip->dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   //printf("  ino   rlen  nlen  name\n");

   while (cp < sbuf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;
     //printf("%4d  %4d  %4d    %s\n", dp->inode, dp->rec_len, dp->name_len, dp->name);
     if (strcmp(temp, name)==0){
        //printf("found %s : ino = %d\n", temp, dp->inode);
        return dp->inode;
     }
     cp += dp->rec_len;
     dp = (DIR *)cp;
   }
   return 0;
}

int getino_dev(char *pathname, int *dev)
{
  int i, ino, blk, offset;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;

  //printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2;
  
  // starting mip = root OR CWD
  if (pathname[0]=='/')
     mip = root;
  else
     mip = running->cwd;

  mip->refCount++;         // because we iput(mip) later
  
  tokenize(pathname);

  for (i=0; i<n; i++){
      //printf("===========================================\n");
      //printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);


      if (mip->ino == 2 && strcmp(name[i], "..") == 0) {
         MTABLE *mt = get_mtable(mip->dev);
         ino = mt->mntDirPtr->ino;
         *dev = mt->mntDirPtr->dev;
      }
      else {
         ino = search(mip, name[i]);
      }

      if (ino==0){
         iput(mip);
         //printf("name %s does not exist\n", name[i]);
         return 0;
      }
      iput(mip);
      mip = iget(*dev, ino);

      if (mip->mounted && strcmp(name[i], "..") != 0) {
         MTABLE *mp = mip->mntPtr;
         mip = iget(mp->dev, 2);
         ino = mip->ino;
         *dev = mp->dev;
      }
   }

   iput(mip);
   return ino;
}

int getino(char *pathname) 
{
   int dev;
   if (pathname[0] == '/') {
      dev = root->dev;
   }
   else {
      dev = running->cwd->dev;
   }

   return getino_dev(pathname, &dev);
}

// These 2 functions are needed for pwd()
int findmyname(MINODE *parent, u32 myino, char myname[ ]) 
{
  // search parent's data block for myino; SAME as search() but by myino
  // copy its name STRING to myname[ ]

   int i; 
   char *cp, c, sbuf[BLKSIZE];
   DIR *dp;
   
   MINODE *mip = parent;

   for (int i = 0; i < 12; i++) {
      if (mip->INODE.i_block[i] == 0) {
         return 0;
      }
       
      get_block(mip->dev, mip->INODE.i_block[i], sbuf);
      dp = (DIR *)sbuf;
      cp = sbuf;

      while (cp < sbuf + BLKSIZE) {
         //printf("%s \n", dp->name);
         if (myino == dp->inode) {
            strcpy(myname, dp->name);
            return 1;
         }
         cp += dp->rec_len;
         dp = (DIR *)cp;
      }
   }

   return 0;
}

int findino(MINODE *mip, u32 *myino, int *dev) // myino = i# of . return i# of ..
{
   // mip points at a DIR minode
   // myino = ino of .  return ino of ..
   // all in i_block[0] of this DIR INODE.


   if (mip->ino == 2) {
      MTABLE *mt = get_mtable(mip->dev);
      mip = mt->mntDirPtr;
      *dev = mip->dev;
   }

   char sbuf[BLKSIZE], *cp;
   DIR *dp;


   get_block(mip->dev, mip->INODE.i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;

   *myino = dp->inode;

   cp += dp->rec_len;
   dp = (DIR *)cp;

   return dp->inode;
}

// tst_bit, set_bit functions
int tst_bit(char *buf, int bit) {
    return buf[bit/8] & (1 << (bit % 8));
}

int set_bit(char *buf, int bit) {
    buf[bit/8] |= (1 << (bit % 8));
}

int decFreeInodes(int dev) {
    // dec free inodes count in SUPER and GD
    char buf[BLKSIZE];
    get_block(dev, 1, buf);
    SUPER *sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    GD *gp = (GD *)buf;
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}


int decFreeBlocks(int dev) {
    // dec free inodes count in SUPER and GD
    char buf[BLKSIZE];
    get_block(dev, 1, buf);
    SUPER *sp = (SUPER *)buf;
    sp->s_free_blocks_count--;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    GD *gp = (GD *)buf;
    gp->bg_free_blocks_count--;
    put_block(dev, 2, buf);
}

MTABLE *get_mtable(int dev) {
   for (int i = 0; i < NMTABLE; i++) {
      if (mtable[i].dev == dev) {
         return &mtable[i];
      }
   }
   return 0;
}


int clr_bit(char *buf, int bit) // clear bit in char buf[BLKSIZE]
{ 
   buf[bit/8] &= ~(1 << (bit%8)); 
}

int incFreeInodes(int dev) {
   char buf[BLKSIZE];
   // inc free inodes count in SUPER and GD
   get_block(dev, 1, buf);
   SUPER *sp = (SUPER *)buf;
   sp->s_free_inodes_count++;
   put_block(dev, 1, buf);
   get_block(dev, 2, buf);
   GD *gp = (GD *)buf;
   gp->bg_free_inodes_count++;
   put_block(dev, 2, buf);
}

int incFreeBlocks(int dev) {
    // dec free inodes count in SUPER and GD
    char buf[BLKSIZE];
    get_block(dev, 1, buf);
    SUPER *sp = (SUPER *)buf;
    sp->s_free_blocks_count++;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    GD *gp = (GD *)buf;
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);
}


/* *  *  *  *  *  *  *  *  *  *  *  *
* Function to verify DIR is empty   *
* returns: 1 if empty               *
*          0 otherwise              *
*  *  *  *  *  *  *  *  *  *  *  *  */
int is_empty(MINODE *mip)
{
   char *cp;
   DIR *dp;
   char buf[BLKSIZE], temp[256];
   INODE *ip;

   ip = &(mip->INODE);

   if (ip->i_links_count > 2) // has more than 2 subdirectory ( '.' & '..') therefore definitely not empty
   {
      return 0;
   }
   else if (ip->i_links_count == 2) // only 2 subdirectory '.' & '..' . But, because files dont increment the i_links_count, we have to check if there are any files in the DIR
   {
      for (int i = 0; i < 12; i++) // traverse the DIR’s data blocks
      {
         if (ip->i_block[i] == 0)
         {
            break;
         }

         get_block(dev, ip->i_block[i], buf);
         dp = (DIR *)buf;
         cp = buf;

         while (cp < buf + BLKSIZE)
         {
            strncpy(temp, dp->name,dp->name_len);
            temp[dp->name_len] = 0;
            
            if(strcmp(temp, ".") && strcmp(temp, ".."))  // if there is a filename other than '.' or '..' . then the Dir is not empty and return 0
            {
               return 0;
            }

            cp += dp->rec_len;
            dp = (DIR *)cp;
         }
      }
   }
   
   return 1;
}

int get_path_and_file(char *input, char *filename, char *filepath) {
   //Tokenize the path and retrieve the basename and dirname
   tokenize(input);
   strcpy(filename, "");
   strcpy(filepath, "");

   for (int i = 0; i < n; i++) {
      if (i == n-1) {
         strcat(filename, name[i]);
      }
      else {
         strcat(filepath, "/");
         strcat(filepath, name[i]);
      }
   }

   if (n == 1) {
      strcpy(filepath, "./");
   }
}



int truncate(MINODE *mip)
{
   for (int i = 0; i < 12; i++) {
      int blk = mip->INODE.i_block[i];
      if (blk == 0) {
         return 0;
      }
      bdalloc(mip->dev, blk);
      mip->INODE.i_block[i] = 0;
   }

   if (mip->INODE.i_block[12] != 0) {

      //Read the indirect block
      int indirectBlk = mip->INODE.i_block[12];
      int ibuf[256];
      get_block(mip->dev, indirectBlk, (char*)ibuf);

      //Deallocate all allocated indirect blocks
      for (int i = 0; i < 256; i++) {
         if (ibuf[i] != 0) {
            bdalloc(mip->dev, ibuf[i]);
         }
      }

      //Deallocate the indirect pointer block
      bdalloc(mip->dev, indirectBlk);
   }

   if (mip->INODE.i_block[13] != 0) {
      int doubleBlk = mip->INODE.i_block[13];

      int ibuf[256];
      int dibuf[256];
      get_block(mip->dev, doubleBlk, (char*)ibuf);

      for (int i = 0; i < 256; i++) {
         if (ibuf[i] != 0) {
            get_block(mip->dev, ibuf[i], (char*)dibuf);
            for (int k = 0; k < 256; k++) {
               if (dibuf[i] != 0) {
                  bdalloc(mip->dev, dibuf[i]);
               }
            }
            bdalloc(mip->dev, ibuf[i]);
         }
      }
      bdalloc(mip->dev, doubleBlk);
   }

   mip->INODE.i_atime = time(0L);
   mip->INODE.i_mtime = time(0L);
   mip->INODE.i_size = 0;
   mip->dirty = 1;
   iput(mip);
}
