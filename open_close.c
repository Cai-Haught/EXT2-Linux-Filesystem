// open_close.c file

int open_file() {
    int i;
    int mode = atoi(parameter);
    MINODE *mip = path2inode(pathname);
    if (!mip) {
        if (mode == 0) 
            return -1;
        creat_file(pathname);
        mip = path2inode(pathname);
        printf("new file = [%d %d]\n", mip->dev, mip->ino);
    }
    if (!S_ISREG(mip->INODE.i_mode)) {
        printf("Not a regular file\n");
        return -1;
    }
    for (i = 0; i < NFD; i++) {
        if (running->fd[i] != NULL) {
            if (running->fd[i]->inodeptr == mip) {
                if (running->fd[i]->mode != 0 || mode != 0) {
                    printf("File open in incompatible mode\n");
                    return -1;
                }
            }
        }
    }
    OFT *oftp = (OFT *)malloc(sizeof(OFT));
    oftp->mode = mode;
    oftp->shareCount = 1;
    oftp->inodeptr = mip;

    switch(mode) {
        case 0: 
            oftp->offset = 0;
            break;
        case 1:
            truncate(mip);
            oftp->offset = 0;
            break;
        case 2: 
            oftp->offset = 0;
            break;
        case 3:
            oftp->offset = mip->INODE.i_size;
            break;
        default:
            printf("invalid mode\n");
            return -1;
    }
    for (i = 0; i < NFD; i++) {
        if (running->fd[i] == NULL) {
            running->fd[i] = oftp;
            mip->INODE.i_atime = time(0L);
            if (mode != 0) 
                mip->INODE.i_mtime = time(0L);
            mip->modified = 1;
            return i;
        }
    }
}

int truncate(MINODE *mip) {
    int i = 0;
    int buf[BLKSIZE], buf2[BLKSIZE];
    if (mip->INODE.i_block[0] != 0) {
        puts("deallocate direct blocks");
        for (i = 0; i < 12; i++) {
            if(mip->INODE.i_block[i] != 0) {
                bdalloc(mip->dev, mip->INODE.i_block[i]);
                printf("%d ", mip->INODE.i_block[i]);
            }
        }
        puts("\n------------------------------");
        getchar();
    }
    if (mip->INODE.i_block[i] != 0) {
        printf("deallocate indirect blocks: block = %d\n", mip->INODE.i_block[i]);
        get_block(mip->dev, mip->INODE.i_block[i], buf);
        for (int j = 0; j < 256; j++) {
            if (buf[j] != 0) {
                bdalloc(mip->dev, buf[j]);
                printf("%d ", buf[j]);
            }
        }
        bdalloc(mip->dev, mip->INODE.i_block[i]);
        puts("\n------------------------------");
        getchar();
    }
    i++;
    if (mip->INODE.i_block[i] != 0) {
        printf("deallocate double indirect blocks: block = %d\n", mip->INODE.i_block[i]);
        get_block(mip->dev, mip->INODE.i_block[i], buf);
        for (int j = 0; j < 256; j++) {
            if (buf[j] != 0) {
                get_block(mip->dev, buf[j], buf2);
                for (int k = 0; k < 256; k++){
                    if (buf2[k] != 0) {
                        bdalloc(mip->dev, buf2[k]);
                        printf("%d ", buf2[k]);
                    }
                }
            }
            bdalloc(mip->dev, buf[j]);
        }
        bdalloc(mip->dev, mip->INODE.i_block[i]);
        puts("\n------------------------------");
        getchar();
    }
    mip->INODE.i_atime = time(0L);
    mip->INODE.i_ctime = time(0L);
    mip->INODE.i_size = 0;
    mip->modified = 1;
}

int close_file(int fd) {
    if (fd < 0 || fd > NFD) {
        printf("file descriptor not in range\n");
        return -1;
    }
    if (running->fd[fd] == 0) {
        printf("invalid file descriptor\n");
        return -1;
    }
    OFT *oftp = running->fd[fd];
    running->fd[fd] = 0;
    oftp->shareCount--;
    if (oftp->shareCount > 0)
        return 0;
    MINODE *mip = oftp->inodeptr;
    iput(mip);
    return 0;
}

int lseek_file(int fd, int position) {
    OFT *oftp = running->fd[fd];
    int old_position = oftp->offset;
    if (position < 0 || position > oftp->inodeptr->INODE.i_size - 1) {
        printf("out of bounds\n");
        return -1;
    }
    oftp->offset = position;
    return old_position;
}

int pfd() {
    puts(" fd     mode       offset    \tINODE");
    puts("----    ----       ------   \t--------");
    for (int i = 0; i < NFD; i++) {
        if (running->fd[i] != 0) {
            printf(" %d      ", i);
            if (running->fd[i]->mode == 0)
                printf("READ       ");
            if (running->fd[i]->mode == 1)
                printf("WRITE      ");
            if (running->fd[i]->mode == 2)
                printf("RDWR       ");
            if (running->fd[i]->mode == 3)
                printf("APPEND     ");
            printf("%d      ", running->fd[i]->offset);
            printf("\t[%d %d]\n", running->fd[i]->inodeptr->dev, running->fd[i]->inodeptr->ino);
        }
    }
}

int dup(int fd) {
    if (running->fd[fd] == NULL) {
        printf("not an opened file descriptor\n");
        return -1;
    }
    for (int i = 0; i < NFD; i++) {
        if (running->fd[i] == NULL) {
            memcpy(running->fd[i], running->fd[fd], sizeof(running->fd[fd]));
            running->fd[i]->shareCount++;
        }
    }
}

int dup2(int fd, int gd) {
    if (running->fd[gd] != NULL) 
        close_file(gd);
    memcpy(running->fd[fd], running->fd[gd], sizeof(running->fd[gd]));
    running->fd[gd]->shareCount++;
}