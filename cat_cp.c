// cat_cp.c file

int cat_file() {
    char mybuf[BLKSIZE];
    int count = 0;
    int n, fd;
    parameter[0] = '0';
    fd = open_file();
    while (n = myread(fd, mybuf, BLKSIZE)) {
        mybuf[n] = '\0';
        printf("%s", mybuf);
        count += n;
    }
    close_file(fd);
    printf("cat %d bytes\n", count);
}

int cp_file() {
    char buf[BLKSIZE];
    char fname1[BLKSIZE] = {0};
    char fname2[BLKSIZE] = {0}; 
    int n, fd, gd;
    strncpy(fname1, pathname, strlen(pathname));
    strncpy(fname2, parameter, strlen(parameter));
    parameter[0] = '0';
    fd = open_file();
    memcpy(pathname, fname2, strlen(fname2));
    parameter[0] = '1';
    gd = open_file();
    while(n = myread(fd, buf, BLKSIZE)) {
        if (mywrite(gd, buf, n)) {
            close_file(fd);
            close_file(gd);
            return -1;
        }
        bzero(buf, strlen(buf));
    }
    close_file(fd);
    close_file(gd);
}

int mv_file() {
    if (pathname == 0) {
        printf("source file not specified\n");
        return -1;
    }
    if (parameter == 0) {
        printf("destination file not specified\n");
        return -1;
    }
    MINODE *src = path2inode(pathname);
    if (src == 0) {
        printf("source file: %s does not exist\n", pathname);
        return -1;
    }
    if (src->dev == dev) {
        link();
        unlink();
    }
    else {
        cp_file();
        unlink();
    }

}