// alloc_dalloc.c file

extern int ninodes, nblocks, imap, bmap;

int tst_bit(char *buf, int bit) {
    return buf[bit/8] & (1 << (bit % 8));
}

int set_bit(char *buf, int bit) {
    buf[bit/8] |= (1 << (bit % 8));
}

int clr_bit(char *buf, int bit) {
    buf[bit/8] &= ~(1 << (bit%8)); 
}

int decFreeInodes(int dev) {
  char buf[BLKSIZE];

  // dec free inodes count by 1 in SUPER and GD
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}

int ialloc(int dev)  // allocate an inode number from imap block
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       put_block(dev, imap, buf);
		
       decFreeInodes(dev);

       printf("ialloc : ino=%d\n", i+1);		
       return i+1;
    }
  }
  return 0;
}

int balloc(int dev) {
    int  i;
    char buf[BLKSIZE];

    get_block(dev, bmap, buf);

    for (i=0; i < nblocks; i++) {
        if (tst_bit(buf, i)==0) {
            set_bit(buf, i);
            put_block(dev, bmap, buf);

            decFreeInodes(dev);
            printf("balloc : bno=%d\n", i+1);
            return i+1;
        }
    }
}

int incFreeInodes(int dev)
{
  char buf[BLKSIZE];

  // inc free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf);
}

int idalloc(int dev, int ino)  // deallocate an ino number
{
    int i;  
    char buf[BLKSIZE];

    // return 0 if ino < 0 OR > ninodes
    if (ino > ninodes || ino < 0)
        return 0;
    // get inode bitmap block
    get_block(dev, imap, buf);
    clr_bit(buf, ino-1);

    // write buf back
    put_block(dev, imap, buf);

    // update free inode count in SUPER and GD
    incFreeInodes(dev);
}

int bdalloc(int dev, int blk) {
    int i;  
    char buf[BLKSIZE];
    if (blk > nblocks || blk < 0)
        return 0;
    // get inode bitmap block
    get_block(dev, bmap, buf);
    clr_bit(buf, blk-1);

    // write buf back
    put_block(dev, bmap, buf);

    // update free inode count in SUPER and GD
    incFreeInodes(dev);
}