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


int enter_name(MINODE *pip, int ino, char *name) {

    char buf[BLKSIZE];
    int i = 0;

    MINODE *parent;
    DIR *dp;
    char *cp;
    int blk;

    //Get the ideal length for the new block
    int need_length = 4*((8 + strlen(name) + 3) / 4);
    
    //Iterate through the parent MINODE disk blocks, assume only 12
    for (i = 0; i < 12; i++) {

        blk = pip->INODE.i_block[i];

        //If no block is available, break and create one
        if (blk == 0) {
            break;
        }

        //Get the parent MINODE at block i in the parent
        get_block(dev, blk, buf);
        dp = (DIR*)buf;
        cp = buf;
        
        //Move to the last entry in the block
        while (cp + dp->rec_len < buf + BLKSIZE) {
            cp += dp->rec_len;
            dp = (DIR*)cp;
        }

        //Get the ideal length for the current last directory
        int ideal_length = 4*((8 + strlen(dp->name) + 3) / 4);
        
        //The remaining length is the rec_length after removing the ideal_length
        int remain = dp->rec_len - ideal_length;
        
        //If the block has enough space to fit the new dir
        if (remain >= need_length) {
            
            //Trim the current last entry rec_len to it's ideal_length
            dp->rec_len = ideal_length;

            //Advance to the next dir, which will be our new directory
            cp += dp->rec_len;
            dp = (DIR*)cp;

            //Set the dir properties
            dp->inode = ino;
            dp->rec_len = remain; //The new remaining length is the current remainder + the new dir length
            dp->name_len = strlen(name); 
            strcpy(dp->name, name);

            //Update the block and return
            put_block(dev, blk, buf);
            return 0;
        }
    }

    //Allocate a new block, i will be the first index where i_block == 0
    blk = balloc(pip->dev);
    pip->INODE.i_block[i] = blk;
    pip->INODE.i_size += BLKSIZE;
    pip->dirty = 1;

    //Get the new block DIR
    get_block(pip->dev, ino, buf);
    dp = (DIR*)buf;
    cp = buf;

    //Set the dir properties
    dp->inode = ino;
    dp->rec_len = BLKSIZE;
    dp->name_len = strlen(name);
    strcpy(dp->name, name);

    //Add the new block
    put_block(pip->dev, blk, buf);

    return 0;
}

int kmkdir(MINODE *pmip, char *basedirname) {

    int dev = pmip->dev;

    //1.
    int ino = ialloc(dev);
    int blk = balloc(dev);

    //2.
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    mip->ino = ino;
    ip->i_mode = 0x41ED; // 040755: DIR type and perm
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->gid; // group Id
    ip->i_size = BLKSIZE; // size in bytes
    ip->i_links_count = 2; // links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2; // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = blk; // new DIR has one data block    
    for (int i = 1; i < 15; i++) { ip->i_block[i] = 0; } //ip->i_block[1] to ip->i_block[14] = 0;
    mip->dirty = 1; // mark minode dirty
    iput(mip); // write INODE to disk
    
    char buf[BLKSIZE];
    bzero(buf, BLKSIZE); // optional: clear buf[ ] to 0
    DIR *dp = (DIR *)buf;
    // make . entry
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';

    // make .. entry: pino=parent DIR ino, blk=allocated block
    dp = (char *)dp + 12;
    dp->inode = pmip->ino;
    dp->rec_len = BLKSIZE-12; // rec_len spans block
    dp->name_len = 2;
    dp->name[0] = dp->name[1] = '.';
    put_block(dev, blk, buf); // write to blk on diks
    
    enter_name(pmip, ino, basedirname);

    //5.
    pmip->INODE.i_links_count += 1;
    pmip->dirty = 1;
    iput(pmip);
}

int my_mkdir(char *pathname) {

    int dev;

    if (pathname[0] == '/') {
        dev = root->dev;
    }
    else {
        dev = running->cwd->dev;
    }

    char dirname[128];
    char dirpath[128];

    get_path_and_file(pathname, &dirname, &dirpath);

    printf("\nDIRBASENAME: %s \n", dirname);
    printf("DIRPATH: %s \n", dirpath);

    //Get the MINODE and make sure it is a directory
    int pino = getino(dirpath);
    MINODE *pmip = iget(dev, pino);

    if (!S_ISDIR(pmip->INODE.i_mode)) {
        printf("PMIP is not a valid directory for path: %s \n", dirpath);
        return -1;
    }

    int s = search(pmip, dirname);
    if (s != 0) {
        printf("Search found existing basename: %s \n", dirname);
        return -1;
    }

    int result = kmkdir(pmip, dirname);
    return result;
}

int kcreat(MINODE *pmip, char *basedirname) {

    //1.
    int ino = ialloc(dev);

    //2.
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x81AE; //0644; //0x41ED; // 040755: DIR type and permissions
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->gid; // group Id
    ip->i_size = 0; //BLKSIZE; // size in bytes
    ip->i_links_count = 1; // links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 0; // LINUX: Blocks count in 512-byte chunks 
    for (int i = 0; i < 15; i++) { ip->i_block[i] = 0; } //ip->i_block[1] to ip->i_block[14] = 0;
    mip->dirty = 1; // mark minode dirty
    iput(mip); // write INODE to disk

    enter_name(pmip, ino, basedirname);
}

int my_creat(char *pathname) {

    char dirname[128];
    char dirpath[128];

    get_path_and_file(pathname, &dirname, &dirpath);

    printf("\nDIRBASENAME: %s \n", dirname);
    printf("DIRPATH: %s \n", dirpath);

    //Get the MINODE and make sure it is a directory
    int pino = getino(dirpath);
    MINODE *pmip = iget(dev, pino);
    if (!S_ISDIR(pmip->INODE.i_mode)) {
        printf("PMIP is not a valid directory for path: %s \n", dirpath);
        return -1;
    }

    int s = search(pmip, dirname);
    if (s != 0) {
        printf("Search found existing basename: %s \n", dirname);
        return -1;
    }

    int result = kcreat(pmip, dirname);
    return result;
}