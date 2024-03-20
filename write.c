// write.c file

int write_file() {
    int fd, nbytes;
    char buf[BLKSIZE];
    if (pathname == '\0') {
        printf("write fd text --- missing file descriptor\n");
        return -1;
    }
    if (parameter == '\0') {
        printf("write fd text --- missing input text\n");
        return -1;
    }
    fd = atoi(pathname);
    if (fd < 0 || fd > NFD) {
        printf("invalid file descriptor\n");
        return -1;
    }
    if (running->fd[fd]->mode == 0) {
        printf("file not opened for W or RW or APPEND\n");
        return -1;
    }
    nbytes = strlen(parameter);
    memcpy(buf, parameter, nbytes);
    return mywrite(fd, buf, nbytes);
}

int mywrite(int fd, char buf[], int nbytes) {
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->inodeptr;
    int count = 0;
    int lbk, start_byte, blk, remain;
    char *cq = buf;
    int ibuf[256], jbuf[256];
    char wbuf[BLKSIZE];
    while (nbytes > 0) {
        bzero(wbuf, strlen(wbuf));
        lbk = oftp->offset / BLKSIZE;
        start_byte = oftp->offset % BLKSIZE;
        if (lbk < 12) {
            if (mip->INODE.i_block[lbk] == 0) 
                mip->INODE.i_block[lbk] = balloc(mip->dev);
            blk = mip->INODE.i_block[lbk];
        }
        else if (lbk >= 12 && lbk < 256 + 12) {
            if (mip->INODE.i_block[12] == 0) {
                mip->INODE.i_block[12] = balloc(mip->dev);
            }
            get_block(mip->dev, mip->INODE.i_block[12], ibuf);
            blk = ibuf[lbk-12];
            if (blk == 0) {
                ibuf[lbk-12] = balloc(mip->dev);
                blk = ibuf[lbk-12];
                put_block(mip->dev, mip->INODE.i_block[12], ibuf);
            }
        }
        else {
            if (mip->INODE.i_block[13] == 0) {
                mip->INODE.i_block[13] = balloc(mip->dev);
            }
            get_block(mip->dev, mip->INODE.i_block[13], ibuf);
            int idblk = ibuf[(lbk - 256 - 12) / 256];
            int idoff = (lbk - 256 - 12) % 256;
            if (idblk == 0) {
                ibuf[(lbk - 256 - 12) / 256] = balloc(mip->dev);
                idblk = ibuf[(lbk - 256 - 12) / 256];
                put_block(mip->dev, mip->INODE.i_block[13], ibuf);
            }
            get_block(mip->dev, idblk, jbuf);
            blk = jbuf[idoff];
            if (blk == 0) {
                jbuf[idoff] = balloc(mip->dev);
                blk = jbuf[idoff];
                put_block(mip->dev, idblk, jbuf);
            }         
        }
        get_block(mip->dev, blk, wbuf);
        char *cp = wbuf + start_byte;
        remain = BLKSIZE - start_byte;
        if (remain < nbytes) {
            memcpy(cp, cq, remain);
            count += remain;
            nbytes -= remain;
            oftp->offset += remain;
            if (oftp->offset > mip->INODE.i_size)
                mip->INODE.i_size += remain;
            remain = 0;
        }
        else {
            memcpy(cp, cq, nbytes);
            count += nbytes;
            remain -= nbytes;
            oftp->offset += nbytes;
            if (oftp->offset > mip->INODE.i_size)
                mip->INODE.i_size += nbytes;
            nbytes = 0;
        }
        put_block(mip->dev, blk, wbuf);
    }
    mip->modified = 1;
    printf("wrote %d char into file descriptor fd = %d\n", count, fd);
    return nbytes;
}