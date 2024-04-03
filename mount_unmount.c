#include "type.h"

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MTABLE mtable[NMTABLE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev;
//extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];


int my_mount(char* pathname) {

    if (pathname[0] == 0) {
        printf("Currently mounted: \n");
        MTABLE *table = get_mtable(dev);

        for (int i = 0; i < NMTABLE; i++) {
            if (mtable[i].dev == 0) {
                break;
            }
            printf("%d: %s", i, mtable[i].devName);
        }

        printf("\n");
        return 0;
    }

    char *dirpath = dirname(pathname);
    char *filename = basename(pathname);

    //Get the mount path
    char mountpath[128];
    printf("input mount path: ");
    fgets(mountpath, 128, stdin);
    mountpath[strlen(mountpath) - 1] = 0;

    int i = 0;
    MTABLE *table;

    for (i = 0; i < NMTABLE; i++) {
        if (mtable[i].dev == 0) {
            break;
        }
        if (strcmp(mtable[i].devName, filename)==0) {
            printf("Mount Error: Device is already mounted \n");
            return -1;
        }
    }
    
    int newDev = open(pathname, O_RDWR);
    if (newDev < 0) {
        printf("Mount Error: can't open root device \n");
        return -1;
    }

    char buf[BLKSIZE];
    get_block(newDev, 1, buf);
    SUPER *sp = (SUPER*)buf;

    /* check magic number */
    if (sp->s_magic != SUPER_MAGIC)
    {
        printf("super magic = %x : %s is not an ext2 filesystem\n", sp->s_magic, filename);
        exit(0);
    }
    
    my_mkdir(mountpath);
    int ino = getino(mountpath);
    MINODE *mip = iget(dev, ino);

    if (!S_ISDIR(mip->INODE.i_mode)) {
        printf("Mount Error: NOT S_ISDIR \n");
    }
    
    //Check mooount_point is not busy e.g. someones cwd??
    //if (mip->INODE.)

    MTABLE *mp = &mtable[i];
    mp->dev = newDev;

    //coppy super block info into mtable[0]
    mp->ninodes = sp->s_inodes_count;
    mp->nblocks = sp->s_blocks_count;
    strcpy(mp->devName, filename);
    strcpy(mp->mntName, mountpath);

    get_block(newDev, 2, buf);
    GD *gp = (GD *)buf;
    
    mp->bmap = gp->bg_block_bitmap;
    mp->imap = gp->bg_inode_bitmap;
    mp->iblock = gp->bg_inode_table;
    mp->free_blocks = gp->bg_free_blocks_count;
    mp->free_inodes = gp->bg_free_inodes_count;

    mip->mounted = 1;
    mp->mntDirPtr = mip;
    mip->mntPtr = mp;
    mip->dirty = 1;
    
    //int pino = getino(".");
    //MINODE *pmip = iget(dev, pino);
    //iput(pmip);

    return 0;
}

int my_unmount(char* mountname) {
    
    MINODE *mip;
    int i;
    int found = 0;

    for (i = 0; i < NMTABLE; i++) {
        if (mtable[i].dev == 0) {
            break;
        }
        if (strcmp(mtable[i].mntName, mountname)==0) {
            found = 1;
            break;
        }
    }

    if (found == 0) {
        printf("Unmount Error: mountname not found: %s \n", mountname);
        return -1;
    }

    int position = i;
    mip = mtable[position].mntDirPtr;

    for (int j =0; j < NMINODE; j++)
    {
        if((mtable[position].dev == minode[j].dev) && (minode[j].refCount > 0))
        {
            printf("Cannot unmount. mounted filesystem is BUSY\n");
            return -1;
        }
    }

    mip->mounted = 0;
    iput(mip);

    return 0;

}
