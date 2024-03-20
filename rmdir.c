// rmdir.c file
int rmdir() {
    int pino;
    MINODE *pip;
    char *child;
    char pname[1000];
    strcpy(pname, pathname);
    child = basename(pname);
    MINODE *mip = path2inode(pathname);
    if (strncmp(pathname, ".", 1) == 0 || strncmp(pathname, "..", 2) == 0 || strncmp(pathname, "/", 1) == 0) {
        printf("you cannot remove %s directory\n", pathname);
        iput(mip);
        return -1;
    }
    if (mip == 0) {
        printf("no such directory\n");
        return -1;
    }
    if(mip->INODE.i_links_count > 2) {
        printf("links_count = %d not an empty directory\n", mip->INODE.i_links_count);
        iput(mip);
        return -1;
    }
    if (is_empty_dir(mip) != 0) {
        printf("not an empty directory (file in dir)\n");
        iput(mip);
        return -1;
    }

     if (!S_ISDIR(mip->INODE.i_mode)) {
        printf("Not a directory\n");
        iput(mip);
        return -1;
    }

    pino = findino(mip, mip->ino);
    for (int i = 0; i < 12; i++) {
        if (mip->INODE.i_block[i] == 0)
            continue;
        bdalloc(mip->dev, mip->INODE.i_block[i]);
    }
    idalloc(mip->dev, mip->ino);
    iput(mip);
    pip = iget(dev, pino);
    rm_child(pip, child);
    pip->INODE.i_links_count--;
    pip->INODE.i_atime = pip->INODE.i_mtime = time(0L);
    pip->modified = 1;
    iput(pip);
}

int rm_child(MINODE *parent, char *name) {
    char buf[BLKSIZE] = {0};
    char *cp, *cp_prev;
    DIR *dp, *dp_prev;
    if(search(parent, name) == 0) {
        printf("no such entry in directory\n");
        return -1;
    }
    for (int i = 0; i < 12; i++) {
        if (parent->INODE.i_block[i] == 0)
            return;
        get_block(dev, parent->INODE.i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;
        dp_prev = (DIR *)buf;
        cp_prev = buf;
         while(cp < buf + BLKSIZE) {
            if (strncmp(name, dp->name, dp->name_len) == 0) {
                if (dp->rec_len == BLKSIZE) {
                    bdalloc(parent->dev, parent->INODE.i_block[i]);
                    parent->INODE.i_size -= BLKSIZE;
                    for (int j = i; j < 11; j++) 
                        parent->INODE.i_block[j] = parent->INODE.i_block[j+1];
                    put_block(dev, parent->INODE.i_block[i], buf);
                    return 1;
                }
                else {
                    if (dp->rec_len + cp >= buf + BLKSIZE) {
                        dp_prev->rec_len += dp->rec_len;
                        put_block(dev, parent->INODE.i_block[i], buf);
                        return 1;
                    }
                    else {
                        int rlen = dp->rec_len;
                        dp_prev->rec_len += rlen;
                        put_block(dev, parent->INODE.i_block[i], buf);
                        return 1;
                    }
                }
            }
            dp_prev = (DIR *)cp;
            cp_prev = cp;
            cp += dp->rec_len;
            dp = (DIR *)cp;
         }
    }
}