/****************************************************************************
*                   KCW: mount root file system                             *
*****************************************************************************/
#include "type.h"


MINODE minode[NMINODE], *root;
MTABLE mtable[NMTABLE];
OFT oft[NOFT];
PROC proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 64 components in pathname
int   n;         // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, iblk;
char line[128], cmd[32], pathname[128];


// forward declarations of funcs in main.c
int quit();

int main(int argc, char *argv[])
{
    printf("mounting root \n");
    printf("enter rootdev name (RETURN for diskimage): ");

    fgets(line, 128, stdin);
    line[strlen(line) - 1] = 0;

    if (line[0] == 0) {
        strcpy(line, "diskimage");
    }

    fs_init();
    mount_root(line);

    while (1)
    {
        printf("input command : [ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|readdir|cat|cp|quit] ");
        fgets(line, 128, stdin);
        line[strlen(line) - 1] = 0;

        if (line[0] == 0)
            continue;
        pathname[0] = 0;

        sscanf(line, "%s %s", cmd, pathname);
        printf("cmd=%s pathname=%s\n", cmd, pathname);

        if (strcmp(cmd, "ls")==0)
            my_ls(pathname);

        else if (strcmp(cmd, "cd")==0)
            my_cd(pathname);

        else if (strcmp(cmd, "pwd")==0)
            my_pwd(running->cwd);

        else if (strcmp(cmd, "mkdir")==0)
            my_mkdir(pathname);

        else if (strcmp(cmd, "creat")==0)
            my_creat(pathname);

        else if (strcmp(cmd, "rmdir")==0)
            my_rmdir(pathname);

        else if (strcmp(cmd, "link")==0)
            my_link(pathname);
        
        else if (strcmp(cmd, "unlink")==0)
            my_unlink(pathname);

        else if (strcmp(cmd, "symlink")==0)
            my_symlink(pathname);

        else if (strcmp(cmd, "open")==0)
            open_file(pathname);

        else if (strcmp(cmd, "cat")==0)
            my_cat(pathname);
        
        else if (strcmp(cmd, "cp")==0)
            my_cp(pathname);

        else if (strcmp(cmd, "mount")==0)
            my_mount(pathname);

        else if (strcmp(cmd, "unmount")==0)
            my_unmount(pathname);

        else if (strcmp(cmd, "quit")==0)
            quit();
    }
}

int quit()
{
    int i;
    MINODE *mip;
    for (i = 0; i < NMINODE; i++)
    {
        mip = &minode[i];
        if (mip->refCount > 0)
            iput(mip);
    }
    exit(0);
}

int fs_init() {
    
    int i,j;

    for (i=0; i<NMINODE; i++) { // initialize all minodes as FREE
        minode[i].refCount = 0;
    }
    for (i=0; i<NMTABLE; i++) { // initialize mtable entries as FREE
        mtable[i].dev = 0;
    }
    for (i=0; i<NPROC; i++) { // initialize PROCs
        proc[i].status = READY;
        proc[i].pid = i;
        proc[i].uid = i;
        
        for (j=0; j<NFD; j++) {
            proc[i].fd[j] = 0;
            //p->fd[j] = 0; // all file descriptors are NULL
        }

        proc[i].next = &proc[i+1];
    }
    proc[NPROC - 1].next = &proc[0];
    running = &proc[0]; // P0 runs first
}

int mount_root(char *rootdev) {

    printf("\nMounting Root: %s \n", rootdev);

    int i;
    MTABLE *mp;
    SUPER *sp;
    GD *gd;
    char buf[BLKSIZE];

    dev = open(rootdev, O_RDWR);
    if (dev < 0) {
        printf("panic : can't open root device \n");
        exit(1);
    }

    /* get super block of rootdev */
    get_block(dev, 1, buf);
    sp = (SUPER*)buf;

    /* check magic number */
    if (sp->s_magic != SUPER_MAGIC)
    {
        printf("super magic = %x : %s is not an ext2 filesystem\n", sp->s_magic, rootdev);
        exit(0);
    }

    //fill mount table mtable[0] with rootdev information
    mp = &mtable[0];
    mp->dev = dev;

    //coppy super block info into mtable[0]
    ninodes = mp->ninodes = sp->s_inodes_count;
    nblocks = mp->nblocks = sp->s_blocks_count;
    strcpy(mp->devName, rootdev);
    strcpy(mp->mntName, "/");

    get_block(dev, 2, buf);
    GD *gp = (GD *)buf;

    bmap = mp->bmap = gp->bg_block_bitmap;
    imap = mp->imap = gp->bg_inode_bitmap;
    iblk = mp->iblock = gp->bg_inode_table;
    mp->free_blocks = gp->bg_free_blocks_count;
    mp->free_inodes = gp->bg_free_inodes_count;

    printf("bmp=%d imap=%d iblock= %d\n", bmap, imap, iblk);
    printf("nblocks=%d bfree=%d ninodes=%d ifree=%d\n",nblocks, mp->free_blocks, ninodes, mp->free_inodes);

    //call iget(), which inc minode's refCount
    root = iget(dev, 2);
    mp->mntDirPtr = root; // double link
    root->mntPtr = mp;

    // set proc CWDs
    for (int i=0; i<NPROC; i++) { // set proc’s CWD
        proc[i].cwd = iget(dev, 2); // each inc refCount by 1
    }

    printf("mount : %s mounted on / \n", rootdev);
    return 0;
}