// symlink.c file

int symlink() {
    char *oldname;
    MINODE *mip;
    strcpy(oldname, pathname);
    mip = path2inode(pathname);
    if (mip != 0 && !S_ISDIR(mip->INODE.i_mode) && !S_ISREG(mip->INODE.i_mode)) {
        printf("file does not exist or is not a DIR or a REG file\n");
        return -1;
    }
    mip = path2inode(parameter);
    if (mip != 0) {
        printf("file already exists\n");
        return -1;
    }
    creat_file(parameter);
    mip = path2inode(parameter);
    mip->INODE.i_mode = 0xA1A4;
    mip->modified = 1;
    strncpy(mip->INODE.i_block, oldname, strlen(oldname));
    mip->INODE.i_size = strlen(oldname);
    iput(mip);
}

char *read_link(char *pathname) {
    char *contents;
    MINODE *mip = path2inode(pathname);
    if (!S_ISLNK(mip->INODE.i_mode)) {
        printf("not a link file\n");
        return -1;
    }
    return (char *)(mip->INODE.i_block);
}