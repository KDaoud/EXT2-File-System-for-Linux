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


int my_write(int fd, char buf[ ], int nbytes) 
{
    int blk, lbk, startByte;
    int count = 0;

    OFT *oftp;
    oftp = running->fd[fd];

    MINODE *mip = oftp->minodePtr;

    int ibuf[256];
    int dibuf[256];

    char *cp, *cq;
    cq = (char*)buf;

    while (nbytes > 0 ) {

        //compute LOGICAL BLOCK (lbk) and the startByte in that lbk:

        lbk = oftp->offset / BLKSIZE;
        startByte = oftp->offset % BLKSIZE;

        // DIRECT data blocks
        // indirect and double-indirect blocks.

        if (lbk < 12){                         // direct block
            if (mip->INODE.i_block[lbk] == 0){   // if no data block yet
                mip->INODE.i_block[lbk] = balloc(mip->dev);// MUST ALLOCATE a block
            }
            blk = mip->INODE.i_block[lbk];      // blk should be a disk block now
        }
        else if (lbk >= 12 && lbk < 256 + 12){ // INDIRECT blocks 
            int inblk = mip->INODE.i_block[12];
            // HELP INFO:
            if (inblk == 0){
                //allocate a block for it;
                inblk = balloc(mip->dev);
                mip->INODE.i_block[12] = inblk;

                char zbuf[BLKSIZE];
                bzero(zbuf, BLKSIZE);
                put_block(mip->dev, inblk, zbuf);
            }

            //get i_block[12] into an int ibuf[256];
            get_block(mip->dev, inblk, ibuf);
            blk = ibuf[lbk - 12];

            if (blk == 0) {
                //allocate a disk block;
                //record it in i_block[12];
                blk = balloc(mip->dev);
                ibuf[lbk - 12] = blk;
                put_block(mip->dev, inblk, ibuf);
            }
        }
        else {
            int offset = lbk - (256 + 12); //Offset lbk to start at 0
            int first = offset / 256; //Divide by zero to get the pos in first list (0/256 = 0, 256+n / 256 = 1..2..3)
            int second = offset % 256; //Find the absolute position in the second list (0..255..258 = 0..255..2)

            int dinblk = mip->INODE.i_block[13];
            // HELP INFO:
            if (dinblk == 0){
                //allocate a block for it;
                dinblk = balloc(mip->dev);
                mip->INODE.i_block[13] = dinblk;

                char zbuf[BLKSIZE];
                bzero(zbuf, BLKSIZE);
                put_block(mip->dev, dinblk, zbuf);
            }

            //get i_block[12] into an int ibuf[256];
            get_block(mip->dev, dinblk, dibuf);
            blk = dibuf[first];

            if (blk == 0) {
                blk = balloc(mip->dev);
                dibuf[first] = blk;
                put_block(mip->dev, dinblk, dibuf);
            }
            
            get_block(mip->dev, blk, ibuf);
            int pblk = blk;
            blk = ibuf[second];

            if (blk == 0) {
                blk = balloc(mip->dev);
                ibuf[second] = blk;
                put_block(mip->dev, pblk, ibuf);
                put_block(mip->dev, dinblk, dibuf);
            }
        }

        char wbuf[BLKSIZE];

        /* all cases come to here : write to the data block */
        get_block(mip->dev, blk, wbuf);   // read disk block into wbuf[ ]  

        if (wbuf == 0) {
            memset(wbuf, BLKSIZE, 0);
        }

        cp = wbuf + startByte;      // cp points at startByte in wbuf[]
        int remain = BLKSIZE - startByte;     // number of BYTEs remain in this block

        while (remain > 0){               // write as much as remain allows  
            *cp++ = *cq++;              // cq points at buf[ ]
            nbytes--; remain--;         // dec counts
            oftp->offset++;             // advance offset
            if (oftp->offset > mip->INODE.i_size)  // especially for RW|APPEND mode
                mip->INODE.i_size++;    // inc file size (if offset > fileSize)
            if (nbytes <= 0) break;     // if already nbytes, break
        }
        put_block(mip->dev, blk, wbuf);   // write wbuf[ ] to disk
    }
    
    mip->dirty = 1;
    printf("wrote %d char into file descriptor fd=%d\n", nbytes, fd);           
    return nbytes;
}