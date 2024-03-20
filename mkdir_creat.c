// mkdir_creat.c file

int make_dir() {
    int c_ino;
    char *parent;
    char *child;

    char pname[1000];
    strcpy(pname, pathname);
    parent = dirname(pname);
    strcpy(pname, pathname);
    child = basename(pname);

    MINODE *pip = path2inode(parent);

    if (pip == 0) {
        printf("Error: pip is NULL\n");
        return -1;
    }

    if (!S_ISDIR(pip->INODE.i_mode)) {
        printf("Not a directory\n");
        return -1;
    }

    c_ino = search(pip, child);
    if (c_ino != 0) {
        printf("%s already exists in the directory\n", child);
        return -1;
    }

    mymkdir(pip, child);
    pip->INODE.i_links_count++;
    pip->INODE.i_atime = time(0L);
    pip->modified = 1;
    iput(pip);
}

int mymkdir(MINODE *pip, char *name) {
    MINODE *mip;
    char buf[BLKSIZE] = {0};
    int ino, bno;
    ino = ialloc(dev);
    bno = balloc(dev);
    printf("ino = %d bno = %d\n", ino, bno);
    mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x41ED;
    ip->i_uid = running->uid;
    ip->i_gid = running->gid;
    ip->i_size = BLKSIZE;
    ip->i_links_count = 2;
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2;
    ip->i_block[0] = bno;
    for (int i = 1; i < 15; i++)
        ip->i_block[i] = 0;
    mip->modified = 1;
    iput(mip);
    DIR *dp = (DIR *)buf;
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';
    dp = (char *)dp + 12;
    dp->inode = pip->ino;
    dp->rec_len = BLKSIZE-12;
    dp->name_len = 2;
    memcpy(dp->name, "..", 2);
    put_block(dev, bno, buf);
    enter_child(pip, ino, name);
}

int enter_child(MINODE *pip, int myino, char *myname) {
    int NEED_LEN, IDEAL_LEN, REMAIN;
    int bno, i;
    char buf[BLKSIZE];
    char *cp;
    NEED_LEN = 4 * ((8 + strlen(myname) + 3) / 4);
    for (i = 0; i < 12; i++) {
        if(pip->INODE.i_block[i] == 0)
            break;
        get_block(dev, pip->INODE.i_block[i], buf);
        DIR *dp = (DIR *)buf;
        cp = buf;
        while(cp + dp->rec_len < buf + BLKSIZE) {
            cp += dp->rec_len;
            dp = (DIR *)cp;
            IDEAL_LEN = 4 * ((8 + dp->name_len + 3) / 4);
            REMAIN = dp->rec_len - IDEAL_LEN;
            if (REMAIN >= NEED_LEN) {
                dp->rec_len = IDEAL_LEN;
                cp += dp->rec_len;
                dp = (DIR *)cp;
                dp->rec_len = REMAIN;
                dp->inode = myino;
                dp->name_len = strlen(myname);
                strncpy(dp->name, myname, strlen(myname));
                put_block(dev, pip->INODE.i_block[i], buf);
                return 1;
            }
        }
    }
    bno = balloc(dev);
    memset(buf, 0, BLKSIZE);
    DIR *dp = (DIR *)buf;
    pip->INODE.i_size += BLKSIZE;
    dp->rec_len = BLKSIZE;
    dp->inode = pip->ino;
    dp->name_len = strlen(myname);
    strncpy(dp->name, myname, strlen(myname));
    put_block(dev, bno, buf);
}

int creat_file(char *pathname) {
    int c_ino;
    char *parent;
    char *child;
    char pname[1000];
    strcpy(pname, pathname);
    parent = dirname(pname);
    strcpy(pname, pathname);
    child = basename(pname);

    MINODE *pip = path2inode(parent);

    if (pip == 0) {
        printf("Error: pip is NULL\n");
        return -1;
    }

    c_ino = search(pip, child);
    if (c_ino != 0) {
        printf("%s already exists in the directory\n", child);
    }

    my_creat(pip, child);
    pip->INODE.i_atime = time(0L);
    pip->modified = 1;
    iput(pip);
}

int my_creat(MINODE *pip, char *name) {
    MINODE *mip;
    char buf[BLKSIZE] = {0};
    int ino, bno;
    ino = ialloc(dev);
    bno = balloc(dev);
    printf("ino = %d bno = %d\n", ino, bno);
    mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x81A4;
    ip->i_uid = running->uid;
    ip->i_gid = running->gid;
    ip->i_size = 0;
    ip->i_links_count = 1;
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2;
    ip->i_block[0] = 0;
    for (int i = 1; i < 15; i++)
        ip->i_block[i] = 0;
    mip->modified = 1;
    iput(mip);
    put_block(dev, bno, buf);
    enter_child(pip, ino, name);
}