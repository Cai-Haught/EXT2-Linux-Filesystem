// read.c file

int read_file() {
    int fd, nbytes;
    if (pathname == '\0'){
        printf("read fd nbytes --- missing file descriptor\n");
        return -1;
    }
    if (parameter == '\0') {
        printf("read fd nbytes --- missing number of bytes\n");
        return -1;
    }
    fd = atoi(pathname);
    nbytes = atoi(parameter);
    if (fd < 0 || fd > NFD) {
        printf("invalid file descriptor\n");
        return -1;
    }
    if (nbytes < 0) {
        printf("invalid number of bytes\n");
        return -1;
    }
    if (running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2) {
        printf("file not opened for R or RW\n");
        return -1;
    }
    char buf[nbytes];
    int n = myread(fd, buf, nbytes);
    buf[n] = '\0';
    printf("%s\n", buf);
    printf("actual number of bytes read: %d\n", n);
    return n;
}

int myread(int fd, char buf[], int nbytes) {
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->inodeptr;
    int count = 0;
    int avil, lbk, start_byte, blk, remain;
    avil = mip->INODE.i_size - oftp->offset;
    char *cq = buf;
    int ibuf[BLKSIZE], jbuf[BLKSIZE]; 
    char readbuf[BLKSIZE];
    while(nbytes && (avil > 0)) {
        lbk = oftp->offset / BLKSIZE;
        start_byte = oftp->offset % BLKSIZE;
        if (lbk < 12) {
            blk = mip->INODE.i_block[lbk];
        }
        else if (lbk >= 12 && lbk < 256 + 12) {
            get_block(mip->dev, mip->INODE.i_block[12], ibuf);
            blk = ibuf[lbk-12];
        }
        else {
            get_block(mip->dev, mip->INODE.i_block[13], ibuf);
            int idblk = ibuf[(lbk - 256 - 12) / 256];
            int idoff = (lbk - 256 - 12) % 256;
            get_block(mip->dev, idblk, jbuf);
            blk = jbuf[idoff];
        }
        get_block(mip->dev, blk, readbuf);
        char *cp = readbuf + start_byte;
        remain = BLKSIZE - start_byte;
        if (avil < remain) 
                remain = avil;
        if (nbytes <= remain) {
            memcpy(cq, cp, nbytes);
            cq += nbytes;
            cp += nbytes;
            oftp->offset += nbytes;
            count += nbytes;
            avil -= nbytes;
            nbytes = 0;
            remain -= nbytes;
        }
        else {
            memcpy(cq, cp, remain);
            cq += remain;
            cp += remain;
            oftp->offset += remain;
            count += remain;
            avil -= remain;
            nbytes -= remain;
            remain = 0;
        }
    }
    printf("\nmyread: read %d char from file descriptor %d\n", count, fd);
    return count;
}