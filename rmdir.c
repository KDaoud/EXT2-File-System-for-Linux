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


int my_rmdir(char *pathname) {

    //Make sure the path is valid
    if (strcmp(pathname, "./") == 0 || strcmp(pathname, "../") == 0 || strcmp(pathname, ".") == 0 || strcmp(pathname, "..") == 0) {
        printf("RMDIR Error: Invalid path, %s \n", pathname);
    }

    //1.
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);
    
    //2.
    if (!S_ISDIR(mip->INODE.i_mode)) {
        printf("MIP is not a valid directory for path: %s \n", pathname);
        return -1;
    }    
    if (!is_empty(mip)) {
        printf("RMDIR Error: DIR is not empty, %s \n", pathname);
        return -1;
    }

    //3. get parent's ino nad inode
    int my_ino;
    int *dev = &mip->dev;
    int pino = findino(mip, &my_ino, dev);
    MINODE *pmip = iget(*dev, pino);
    
    //4. get name from paren DIR's data block
    char buf[BLKSIZE];
    findmyname(pmip, ino, buf);
    
    //5.
    rm_child(pmip, buf);

    //6.
    pmip->INODE.i_links_count -= 1;
    pmip->dirty = 1;
    iput(pmip);

    //7.
    bdalloc(mip->dev, mip->INODE.i_block[0]);
    idalloc(mip->dev, mip->ino);
    iput(mip);

    return 0;
}

int rm_child(MINODE *pmip, char *name)
{
    
    char *cp, *prevcp;
    DIR *dp, *prevdp = 0;
    char buf[BLKSIZE];
    int test = 0;

    INODE* ip = &(pmip->INODE);

    //Search parent INODE’s data block(s) for the entry of name
    for (int i=0; i< 12; i++) {

        int blk = pmip->INODE.i_block[i];
        if (blk == 0) {
            return -1;
        }
        
        get_block(dev, blk, buf);
        dp = (DIR *)buf;
        cp = buf;

        //Iterate through directories in block
        while (cp < buf + BLKSIZE) {
            //If the name is a match
            if(strcmp(name, dp->name) == 0) {

                //first and only entry in a data block
                if (dp->rec_len == BLKSIZE) {
                    
                    bdalloc(dev, blk);

                    //reduce parent size
                    ip->i_size -= BLKSIZE;
                    ip->i_block[11] = 0;

                    for (int j = 0; j < 12; j++) {
                        if (j >= i) {
                            ip->i_block[j] = ip->i_block[j+1];
                        }
                    }

                    put_block(dev, ip->i_block[i-1], buf);
                }
                //LAST entry in block
                else if (cp + dp->rec_len == buf + BLKSIZE) {
                    //absorbs its rec_len to the predecessor
                    prevdp->rec_len += dp->rec_len;
                    put_block(dev, blk, buf);
                }
                //entry is first but not the only entry or in the middle of a block
                else {
                    
                    int del_len = dp->rec_len;

                    //Move to next to skip
                    cp += dp->rec_len;
                    dp = (DIR*)cp;

                    prevcp += prevdp->rec_len;
                    prevdp = (DIR*)prevcp;

                    //Move to the last entry in the block
                    while (cp + dp->rec_len < buf + BLKSIZE) {

                        memcpy(prevdp, dp, dp->rec_len);
                        
                        prevcp += prevdp->rec_len;
                        prevdp = (DIR*)prevcp;
                        
                        cp += dp->rec_len;
                        dp = (DIR*)cp;
                    }

                    memcpy(prevdp, dp, dp->rec_len);
                    prevdp->rec_len += del_len;
                    put_block(dev, blk, buf);
                }

                return 0;
            }

            prevcp = cp;
            cp += dp->rec_len;
            prevdp = dp;
            dp = (DIR*)cp;
        }
    }
}