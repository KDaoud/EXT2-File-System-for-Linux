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

int my_cp(char *filepath) {

    //Get the new filename
    char newFilename[128];
    printf("input NEW filename: ");
    fgets(newFilename, 128, stdin);
    newFilename[strlen(newFilename) - 1] = 0;

    if (getino(newFilename) != 0) {
        printf("CP Error: File must not already exist, %s \n", newFilename);
        return -1;
    }

    int fd = my_open(filepath, O_RDONLY);
    int gd = my_open(newFilename, O_WRONLY);

    if (fd == -1 || gd == -1) {
        printf("CP Error \n");
        return -1;
    }

    int n;
    char buf[BLKSIZE];
    while (n = myread(fd, buf, BLKSIZE)) {
        my_write(gd, buf, n);
    }

    my_close_file(fd);
    my_close_file(gd);
}