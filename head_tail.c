// head_tail.c file

int head_file() {
    int fd, n;
    int lines = 0;
    int count = 0;
    char buf[BLKSIZE];
    char *cp;
    parameter[0] = '0';
    fd = open_file();
    if (fd < 0) {
        printf("could not open file for READ");
        return -1;
    }
    n = myread(fd, buf, BLKSIZE);
    cp = buf;
    while (lines < 10) {
        if (*cp == '\n') 
            lines++;      
        count++;
        cp++;
    }
    buf[count] = 0;
    printf("%s", buf);
    close_file(fd);
}

int tail_file() {
    int fd, n, size;
    int lines = 0;
    char buf[BLKSIZE] = {0};
    char *cp;
    parameter[0] = '0';
    fd = open_file();
    if (fd < 0) {
        printf("could not open file for READ");
        return -1;
    }
    size = running->fd[fd]->inodeptr->INODE.i_size;
    if (size > BLKSIZE)
        lseek_file(fd, size - BLKSIZE);
    else 
        lseek_file(fd, 0);
    n = myread(fd, buf, BLKSIZE);
    cp = buf;
    cp += n;
    while(lines < 11) {
        if (*cp == '\n')
            lines++;
        cp--;
    }
    buf[n] = 0;
    printf("%s", cp+1);
    close_file(fd);
}