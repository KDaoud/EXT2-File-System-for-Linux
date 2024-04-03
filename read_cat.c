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

int read_file()
{
/*
  Preparations: 
    ASSUME: file is opened for RD or RW;
    ask for a fd  and  nbytes to read;
    verify that fd is indeed opened for RD or RW;
    return(myread(fd, buf, nbytes));
*/

    int fd = 0, n_bytes = 0;
    char buf[BLKSIZE];

    printf("input fd: ");
    scanf("%d", &fd);
    printf("input n_bytes: ");
    scanf("%d", &n_bytes);

    if ( fd < 0 || fd > (NFD -1))
    {
        printf("ERROR: Invalid fd\n");
        return -1;
    }

    /*
    read/write/rw/append modes
    READ 0
    WRITE 1
    READ_WRITE 2
    APPEND 3
    */
    if(running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2)
    {
        printf("ERROR: fd is not open for READ (0) or READ_WRITE (2).\n");
        return -1;
    }

    return myread(fd, buf, n_bytes);
}


int myread(int fd, char *buf, int n_bytes)
{
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->minodePtr;

/*
 1. int count = 0;
    avil = fileSize - OFT's offset // number of bytes still available in file.
    char *cq = buf;                // cq points at buf[ ]
*/
    int lbk, startByte, blk, remain;
    int count =0;
    int avil = mip->INODE.i_size - oftp->offset;
    char *cq = buf;
    char *readbuf[BLKSIZE];

    if (n_bytes > avil) {n_bytes = avil;}

    while (n_bytes && avil)
    {
        /*
        Compute LOGICAL BLOCK number lbk and startByte in that block from offset;
        lbk       = oftp->offset / BLKSIZE;
        startByte = oftp->offset % BLKSIZE;
        */
        lbk = oftp->offset / BLKSIZE;
        startByte = oftp->offset % BLKSIZE;

        //Converting Logical Blocks to Physical Blocks
        blk = map(mip, lbk);

        //get the data block into readbuf[BLKSIZE]
        get_block(mip->dev, blk, readbuf);

        //copy from startByte to buf[ ], at most remain bytes in this block
        char *cp = readbuf + startByte;

        // number of bytes remain in readbuf[]
        remain = BLKSIZE - startByte;   

        if (n_bytes <= remain)
        {
            memcpy(cq, cp, n_bytes);    // copy byte from readbuf[] into buf[]
            cq += n_bytes;
            cp += n_bytes;
            oftp->offset += n_bytes;    // advance offset
            count += n_bytes;           // inc count as number of bytes read
            avil -= n_bytes;
            remain -= n_bytes;
            n_bytes = 0;
        }
        else
        {
            memcpy(cq, cp, remain);    // copy byte from readbuf[] into buf[]
            cq += remain;
            cp += remain;
            oftp->offset += remain;    // advance offset
            count += remain;           // inc count as number of bytes read
            avil -= remain;
            n_bytes -= remain;
            remain = 0;
        }

    }

    printf("myread: read %d char from file descriptor %d\n", count, fd); 
    return count;   // count is the actual number of bytes read
}


//Converting Logical Blocks to Physical Blocks
int map(MINODE *mip, int lbk)
{ 
    // printf("1. lbk: %d\n", lbk); for testing
    char *ibuf[BLKSIZE];     //for indirect blocks
    char *d_ibuf[BLKSIZE];   //for couble indirect blocks
    int blk;
    int *ip;
    // convert lbk to blk via INODE
    if (lbk < 12) // direct blocks
    {
        // direct blocks
        blk = mip->INODE.i_block[lbk];
    }
    else if ((lbk >= 12) && (lbk < 256 + 12))
    { 
        // indirect blocks
        get_block(mip->dev, mip->INODE.i_block[12], ibuf);
        ip = (int *)ibuf + lbk -12;
        blk = *ip;
        // printf("indirect blk: %d", blk); //for testing
    }
    else
    { 
        // doube indirect blocks; see Exercise 11.13 below.
        get_block(mip->dev, mip->INODE.i_block[13], (char *)ibuf);
        

        //mailman's alg
        int chunk_size, i_blk, i_blk_offset;

        chunk_size = BLKSIZE / sizeof(int);
        // printf("chunksize: %d\n", chunk_size); for testing

        i_blk = (lbk - chunk_size -12) / chunk_size;
        i_blk_offset = (lbk - chunk_size -12) % chunk_size;

        ip = (int *)ibuf + i_blk;
        get_block(mip->dev, *ip, d_ibuf);

        ip = (int *)d_ibuf + i_blk_offset;
        blk = *ip;
        // printf(".1..d in blk: %d\n", blk); for testing
    }

    return blk;
}


int my_cat(char *pathname)
{
    char mybuf[BLKSIZE], dummy = 0;  // a null char at end of mybuf[ ]
    int n;

    int fd = my_open(pathname, O_RDONLY);

    if(running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2)
    {
        printf("ERROR: fd is not open for READ. can't CAT.\n");
        // close_file(fd);          //waiting on close_file()
        return -1;
    }

    while(n = myread(fd, mybuf, BLKSIZE))
    {
        mybuf[n] = 0;

        // printf("%s", mybuf); //OBSOLETE improved version below

        //spit out chars from mybuf[ ] and handle \n properly;
        char *cp = mybuf;
        while (*cp != '\0') // '\0' null terminator
        {
            if(*cp == '\n') //  handle new line
            {
                printf("\n");
            }
            else
            {
                printf("%c", *cp);
            }
            cp++;
        }
    }
    
    //3. close(fd);
    my_close_file(fd);
    return 0;
}