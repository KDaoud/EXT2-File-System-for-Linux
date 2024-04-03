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

int my_open(char *filename, int flags) {

   int ino = getino(filename);
   if (ino == 0) {
      my_creat(filename);
      ino = getino(filename);
   }

   MINODE *mip = iget(dev, ino);

   if (!S_ISREG(mip->INODE.i_mode)) {
      printf("OPEN Error: file is not reg: %s \n", filename);
      return -1;
   }

   //Iterate through openn FDs 
   for (int i = 0; i < NFD; i++) {
      //Check if FD at i is equal to the minode
      if (running->fd[i] != 0 && running->fd[i]->minodePtr == mip) {
         int runningMode = running->fd[i]->mode;
         //If in any other mode than read or in read mode and opening in non-read
         if (runningMode > 0 && flags > 0) {
            printf("OPEN Error: File is already opened in mode: %d \n", runningMode);
            return -1;
         }
      }
   }

   //Allocate a new OFT table
   OFT *oftp;
   oftp = malloc(sizeof(OFT));
   oftp->mode = flags;
   oftp->refCount = 1;
   oftp->minodePtr = mip;


   switch(flags){
      case 0 : oftp->offset = 0;     // R: offset = 0
               break;
      case 1 : truncate(mip);        // W: truncate file to 0 size
               oftp->offset = 0;
               break;
      case 2 : oftp->offset = 0;     // RW: do NOT truncate file
               break;
      case 3 : oftp->offset =  mip->INODE.i_size;  // APPEND mode
               break;
      default: printf("invalid mode\n");
               return(-1);
   }

   //Find the lowest open fd
   int i = 0;
   for (i = 0; i < NFD; i++) {
      if (running->fd[i] == 0) {
         break;
      }
   }

   //Update INODE time and mark dirty
   running->fd[i] = oftp;
   if (flags == 0) {
      mip->INODE.i_atime = time(0L);
   }
   else {
      mip->INODE.i_atime = time(0L);
      mip->INODE.i_mtime = time(0L);
   }
   mip->dirty = 1;

   return i;
}

int open_file(char *filename) {

   int mode = 0;
   printf("Input a file mode (0=R, 1=W, 2=RW, 3=APPEND): ");
   scanf("%d", &mode);
   printf("\n");

   if (mode < 0 || mode > 3) {
      printf("OPEN Error: Invalid file mode: %d \n", mode);
      return -1;
   }

   return my_open(filename, mode);
}

int my_close_file(int fd)
{
   if (fd >= NFD) {
      printf("CLOSE Error: fd >= NFD: %d \n", fd);
      return -1;
   }

   OFT *oftp = running->fd[fd];
   if (oftp == 0) {
      printf("CLOSE Error: running->fd[fd] is NULL: %d \n", fd);
      return -1;
   }

   running->fd[fd] = 0;
   oftp->refCount -= 1;

   if (oftp->refCount > 0) {
      return 0;
   }

   MINODE *mip = oftp->minodePtr;
   iput(mip);

   return 0;
}