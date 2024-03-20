/*********** globals in main.c ***********/
extern PROC   proc[NPROC];
extern PROC   *running;

extern MINODE minode[NMINODE];   // minodes
extern MINODE *freeList;         // free minodes list
extern MINODE *cacheList;        // cacheCount minodes list

extern MINODE *root;

extern OFT    oft[NOFT];

extern char gline[256];   // global line hold token strings of pathname
extern char *name[64];    // token string pointers
extern int  n;            // number of token strings                    

extern int ninodes, nblocks;
extern int inode_size, INODEsize, inodes_per_block, ifactor;
extern int bmap, imap, inodes_start, iblk;  // bitmap, inodes block numbers

extern int  fd, dev;
extern char cmd[16], pathname[128], parameter[128];
extern int  requests, hits;

/**************** util.c file **************/


int get_block(int dev, int blk, char buf[ ])
{
  lseek(dev, blk*BLKSIZE, SEEK_SET);
  int n = read(fd, buf, BLKSIZE);
  return n;
}

int put_block(int dev, int blk, char buf[ ])
{
  lseek(dev, blk*BLKSIZE, SEEK_SET);
  int n = write(fd, buf, BLKSIZE);
  return n; 
}       

int tokenize(char *pathname)
{
  // tokenize pathname into n token strings in (global) gline[ ]
    char *s;
    strcpy(gline, pathname);
    s = strtok(gline, "/");
    n = 0;
    while (s)
    {
        name[n++] = s;
        s = strtok(0, "/");
    }
    name[n] = 0;
}

int enqueue(MINODE **list, MINODE *p) {
    MINODE *q = *list;
    if (q == 0 || p->cacheCount < q->cacheCount) {
        *list = p;
        p->next = q;
    }
    else {
        while(q->next && p->cacheCount >= q->next->cacheCount)
            q = q->next;
        p->next = q->next;
        q->next = p;
    }
}

MINODE *dequeue(MINODE **list) {
    MINODE *p = *list;
    if (p)
        *list = (*list)->next;
    return p;
}

MINODE *iget(int dev, int ino) // return minode pointer of (dev, ino)
{
  /********** Write code to implement these ***********
  1. search cacheList for minode=(dev, ino);
  if (found){
     inc minode's cacheCount by 1;
     inc minode's shareCount by 1;
     return minode pointer;
  }

  // needed (dev, ino) NOT in cacheList
  2. if (freeList NOT empty){
        remove a minode from freeList;
        set minode to (dev, ino), cacheCount=1 shareCount=1, modified=0;
 
        load INODE of (dev, ino) from disk into minode.INODE;

        enter minode into cacheList; 
        return minode pointer;
     }
  
  // freeList empty case:
  3. find a minode in cacheList with shareCount=0, cacheCount=SMALLest
     set minode to (dev, ino), shareCount=1, cacheCount=1, modified=0
     return minode pointer;

 NOTE: in order to do 3:
       it's better to order cacheList by INCREASING cacheCount,
       with smaller cacheCount in front ==> search cacheList
  ************/

    MINODE *mip = cacheList;
    int blk, offset;
    char buf[BLKSIZE];
    requests++;
    while (mip) {
        if (mip->dev == dev && mip->ino == ino) {
            mip->cacheCount++;
            mip->shareCount++;
            hits++;
            return mip;
        }
        mip = mip->next;
    }

    mip = dequeue(&freeList);
    if (mip) {
        mip->dev = dev; mip->ino = ino;
        mip->cacheCount = 1; mip->shareCount = 1; mip->modified = 0;
        blk = (ino - 1) / inodes_per_block + iblk;
        offset = (ino - 1) % inodes_per_block;
        get_block(dev, blk, buf);
        INODE *ip = (INODE *)buf + offset*ifactor;
        mip->INODE = *ip;
        enqueue(&cacheList, mip);
        return mip;
    }
    else {
        mip = cacheList;
        do {
            if (mip->shareCount == 0) {
                mip->dev = dev; mip->ino = ino;
                mip->cacheCount = 1; mip->shareCount = 1; mip->modified = 0;
                blk = (ino - 1) / inodes_per_block + iblk;
                offset = (ino - 1) % inodes_per_block;
                get_block(dev, blk, buf);
                INODE *ip = (INODE *)buf + offset*ifactor;
                mip->INODE = *ip;
                return mip;
            }
            reorder(mip);
        } while(1);

    }

}

int iput(MINODE *mip)  // release a mip
{
    /*******************
     1.  if (mip==0)                return;

        mip->shareCount--;         // one less user on this minode

        if (mip->shareCount > 0)   return;
        if (!mip->modified)        return;

    2. // last user, INODE modified: MUST write back to disk
    Use Mailman's algorithm to write minode.INODE back to disk)
    // NOTE: minode still in cacheList;
    *****************/
    int blk, offset;
    char buf[BLKSIZE];
    if (mip == 0)
        return;
    mip->shareCount--;
    if(mip->shareCount > 0)
        return;
    if (!mip->modified)
        return;
    blk = (mip->ino - 1) / inodes_per_block + iblk;
    offset = (mip->ino - 1) % inodes_per_block;
    get_block(dev, blk, buf);
    INODE *ip = (INODE *)buf + offset*ifactor;
    *ip = mip->INODE;
    put_block(mip->dev, blk, buf);
    mip->shareCount = 0;
} 

int search(MINODE *mip, char *name)
{
    /******************
     search mip->INODE data blocks for name:
    if (found) return its inode number;
    else       return 0;
    ******************/
    char *cp, temp[256], sbuf[BLKSIZE];
    DIR *dp;
    for (int i = 0; i < 12; i++) {
        if (mip->INODE.i_block[i] == 0)
            break;
        get_block(mip->dev, mip->INODE.i_block[i], sbuf);
        dp = (DIR *)sbuf;
        cp = sbuf;
        while (cp < sbuf + BLKSIZE) {
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            if (strcmp(name, temp) == 0)
                return dp->inode;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return 0;
}

MINODE *path2inode(char *pathname) 
{
    /*******************
     return minode pointer of pathname;
    return 0 if pathname invalid;

    This is same as YOUR main loop in LAB5 without printing block numbers
    *******************/
    MINODE *mip;
    int ino;
    if (strcmp(pathname, "/") == 0)
        mip = root;
    else
        mip = running->cwd;
    mip->shareCount++;
    tokenize(pathname);
    for (int i = 0; i < n; i++) {
        if (!S_ISDIR(mip->INODE.i_mode)) {
            iput(mip);
            return 0;
        }
        ino = search(mip, name[i]);
        if (ino == 0) {
            iput(mip);
            return 0;
        }
        iput(mip);
        mip = iget(dev, ino);
    }
    return mip;

}   

int findmyname(MINODE *pip, int myino, char myname[ ]) 
{
    /****************
     pip points to parent DIR minode: 
    search for myino;    // same as search(pip, name) but search by ino
    copy name string into myname[256]
    ******************/
    char *cp, temp[256], sbuf[BLKSIZE];
    DIR *dp;
    for (int i = 0; i < 12; i++) {
        if (pip->INODE.i_block[i] == 0)
            break;
        get_block(pip->dev, pip->INODE.i_block[i], sbuf);
        dp = (DIR *)sbuf;
        cp = sbuf;
        while (cp < sbuf + BLKSIZE) {
            if (myino == dp->inode)
                strncpy(myname, dp->name, dp->name_len);
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
}
 
int findino(MINODE *mip, int *myino) 
{
    /*****************
     mip points at a DIR minode
    i_block[0] contains .  and  ..
    get myino of .
    return parent_ino of ..
    *******************/
    char *cp, sbuf[BLKSIZE], temp[256];
    DIR *dp;
    int parent_ino;
    get_block(dev, mip->INODE.i_block[0], sbuf);
    dp = (DIR *)sbuf;
    cp = sbuf;
    while(cp < sbuf + BLKSIZE) {
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;
        if (strcmp(".", temp) == 0)
            myino = dp->inode;
        if (strcmp("..", temp) == 0)
            parent_ino = dp->inode;
        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
    return parent_ino;
}

void reorder(MINODE *mip) {
    MINODE *tmp;
    while (1) {
        mip = dequeue(&cacheList);
        if (mip == 0)
            break;
        enqueue(&tmp, mip);
    }
    cacheList = tmp;
}

int is_empty_dir(MINODE *mip) 
{
    char *cp, sbuf[BLKSIZE], temp[256];
    DIR *dp;
    int parent_ino;
    get_block(dev, mip->INODE.i_block[0], sbuf);
    dp = (DIR *)sbuf;
    cp = sbuf;
    while(cp < sbuf + BLKSIZE) {
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;
        if ((strncmp(".", temp, 1) != 0) || (strncmp("..", temp, 1) != 0))
            return -1;
        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
    return 0;
}