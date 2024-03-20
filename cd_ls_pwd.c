// cd_ls_pwd.c file

int cd()
{
    // write YOUR code for cd
    MINODE *mip = path2inode(pathname);
    if (mip != 0) {
        if (S_ISDIR(mip->INODE.i_mode)) {
            printf("cd to [dev ino] = [%d %d]\n", dev, mip->ino);
            iput(running->cwd);
            running->cwd = mip;
            printf("after cd : cwd = [%d %d]\n", running->cwd->dev, running->cwd->ino);
        }
        else
            printf("cd : not a directory %s\n", pathname);
    }
    else 
        printf("cd : no such directory %s\n", pathname);
}

int ls_file(MINODE *mip, char *name)
{
  // use mip->INODE to ls_file
    char *t1 = "xwrxwrxwr-------";
    char *t2 = "----------------";
    char ftime[64];
    if ((mip->INODE.i_mode & 0xF000) == 0x8000)
        printf("%c",'-');
    if ((mip->INODE.i_mode & 0xF000) == 0x4000) 
        printf("%c",'d');
    if ((mip->INODE.i_mode & 0xF000) == 0xA000) 
        printf("%c",'l');
    for (int i = 8; i>=0; i--) {
        if (mip->INODE.i_mode & (1 << i)) 
            printf("%c", t1[i]);
        else
            printf("%c", t2[i]); 
    }
    printf("%4d ",mip->INODE.i_links_count); 
    printf("%4d ",mip->INODE.i_gid); 
    printf("%4d ",mip->INODE.i_uid); 
    time_t time = mip->INODE.i_ctime;
    strcpy(ftime, ctime(&time)); 
    ftime[strlen(ftime)-1] = 0; 
    printf("%25s ",ftime);
    printf("%8d ",mip->INODE.i_size); 
    printf("%8s", basename(name)); 
    if ((mip->INODE.i_mode & 0xF000)== 0xA000)
        printf(" -> %s", (char *)(mip->INODE.i_block)); // print linked name
    printf("\t[%d %d]", dev, mip->ino);
    printf("\n");
}
  
int ls_dir(MINODE *pip)
{
    char sbuf[BLKSIZE], name[256];
    DIR  *dp;
    char *cp;
    MINODE *mip;   
    printf("i_block[0] = %d\n", pip->INODE.i_block[0]);
    get_block(dev, pip->INODE.i_block[0], sbuf);
    dp = (DIR *)sbuf;
    cp = sbuf;
    while (cp < sbuf + BLKSIZE){
        strncpy(name, dp->name, dp->name_len);
        name[dp->name_len] = 0;
        mip = iget(dev, dp->inode);
        ls_file(mip, name);
        iput(mip);
        cp += dp->rec_len;
        dp = (DIR *)cp;
    }

}

int ls()
{
    MINODE *mip;
    if (pathname[0] == '\0')
        mip = running->cwd;
    else {
        printf("ls %s\n", pathname);
        mip = path2inode(pathname);
    }
    if (mip != 0) {
        if (S_ISDIR(mip->INODE.i_mode))
            ls_dir(mip);
        else
            ls_file(mip, basename(pathname));
        iput(mip);
    }
    else
        printf("ls : no such file %s\n", pathname);
}

int pwd()
{
    MINODE *wd = running->cwd;
    if (wd == root)
       puts("/");
    else {
        rpwd(wd);
        printf("\n");
    }
}

int rpwd(MINODE *wd) {
    int myino = wd->ino;
    int pino;
    MINODE *pip;
    char myname[256];
    if (wd == root) 
        return;
    pino = findino(wd, myino);
    pip = iget(dev, pino);
    findmyname(pip, myino, myname);
    rpwd(pip);
    iput(pip);
    printf("/%s", myname);
}


