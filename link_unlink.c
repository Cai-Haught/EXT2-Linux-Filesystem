// link_unlink.c file

int link() {
    char *parent;
    char *child;
    char pname[1000];

    MINODE *mip = path2inode(pathname);
    if (S_ISDIR(mip->INODE.i_mode)) {
        printf("cannot link to a directory\n");
        return -1;
    }

    strcpy(pname, parameter);
    parent = dirname(pname);
    strcpy(pname, parameter);
    child = basename(pname);
 
    MINODE *pip = path2inode(parent);
    if (pip != 0 && S_ISDIR(pip->INODE.i_mode)) {
        if (search(pip, child) == 0) {
            enter_child(pip, mip->ino, child);
            mip->INODE.i_links_count++;
            mip->modified = 1;
            iput(mip);
            iput(pip);
        }
    }
}

int unlink() {
    char *parent;
    char *child;
    char pname[1000];
    strcpy(pname, pathname);
    parent = dirname(pname);
    strcpy(pname, pathname);
    child = basename(pname);
    MINODE *mip = path2inode(pathname);
    MINODE *pip = path2inode(parent);
    if (!S_ISREG(mip->INODE.i_mode) && !S_ISLNK(mip->INODE.i_mode)) {
        printf("not a REG or LNK file\n");
        return -1;
    }
    mip->INODE.i_links_count--;
    if (mip->INODE.i_links_count == 0) {
        truncate(mip);
        idalloc(mip->dev, mip->ino);
    }
    else{
        mip->modified = 1;
    }
    rm_child(pip, child);
    pip->modified = 1;
    iput(pip);
    iput(mip);
}
