#include "type.h"

/********** globals **************/
PROC   proc[NPROC];
PROC   *running;

MINODE minode[NMINODE];   // in memory INODES
MINODE *freeList;         // free minodes list
MINODE *cacheList;        // cached minodes list

MINODE *root;             // root minode pointer

OFT    oft[NOFT];         // for level-2 only

char gline[256];          // global line hold token strings of pathname
char *name[64];           // token string pointers
int  n;                   // number of token strings                    

int ninodes, nblocks;     // ninodes, nblocks from SUPER block
int inode_size, INODEsize, inodes_per_block, ifactor;
int bmap, imap, inodes_start, iblk;  // bitmap, inodes block numbers

int  fd, dev;
char cmd[16], pathname[128], parameter[128];
int  requests, hits;

// start up files
#include "util.c"
#include "cd_ls_pwd.c"
#include "alloc_dalloc.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "link_unlink.c"
#include "symlink.c"
#include "open_close.c"
#include "read.c"
#include "write.c"
#include "cat_cp.c"
#include "head_tail.c"

int init()
{
  int i, j;
  // initialize minodes into a freeList
  for (i=0; i<NMINODE; i++){
    MINODE *mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->id = i;
    mip->next = &minode[i+1];
  }
  minode[NMINODE-1].next = 0;
  freeList = &minode[0];       // free minodes list

  cacheList = 0;               // cacheList = 0

  for (i=0; i<NOFT; i++)
    oft[i].shareCount = 0;     // all oft are FREE
 
  for (i=0; i<NPROC; i++){     // initialize procs
     PROC *p = &proc[i];    
     p->uid = p->gid = i;      // uid=0 for SUPER user
     p->pid = i+1;             // pid = 1,2,..., NPROC-1

     for (j=0; j<NFD; j++)
       p->fd[j] = 0;           // open file descritors are 0
  }
  
  running = &proc[0];          // P1 is running
  requests = hits = 0;         // for hit_ratio of minodes cache
}

char *disk = "diskimage";

int main(int argc, char *argv[ ]) 
{
  char line[128];
  char buf[BLKSIZE];

  init();
  
  fd = dev = open(disk, O_RDWR);
  printf("dev = %d\n", dev);  // YOU should check dev value: exit if < 0

  // get super block of dev
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;  // you should check s_magic for EXT2 FS
  
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;
  inode_size = sp->s_inode_size;
  INODEsize = sizeof(INODE);
  inodes_per_block = BLKSIZE / inode_size;
  ifactor = inode_size / INODEsize;
  printf("check: superblock magic = 0xef53  ");
  if (sp->s_magic == 0xEF53)
    printf("OK\n");
  else 
    printf("FAIL\n");
  printf("ninodes=%d  nblocks=%d  inode_size=%d\n", ninodes, nblocks, sp->s_inode_size);
  printf("inodes_per_block=%d  ifactor=%d\n", inodes_per_block, ifactor);
  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = inodes_start = gp->bg_inode_table;

  printf("bmap=%d  imap=%d  iblk=%d\n", bmap, imap, iblk);

  root         = iget(dev, 2);  
  running->cwd = iget(dev, 2);
  printf("root shareCount=%d\n", root->shareCount);
  while(1){
     printf("P%d running\n", running->pid);
     pathname[0] = parameter[0] = 0;
     // level 1 commands
    //  printf("enter command [cd|ls|pwd|mkdir|creat|rmdir|link|unlink|symlink|show|hits|exit] : ");
     // level 2 commands
     printf("enter command [ls|open|close|pfd|read|write|cat|cp|head|tail] : ");
     fgets(line, 128, stdin);
     line[strlen(line)-1] = 0;    // kill \n at end

     if (line[0]==0)
        continue;
     for (int i = 0; i < 128; i++) {
        parameter[i] = 0;
        pathname[i] = 0;
     } 
     sscanf(line, "%s %s %s", cmd, pathname, parameter);
     printf("pathname=%s parameter=%s\n", pathname, parameter);
      
    if (strcmp(cmd, "ls")==0)
        ls();
    if (strcmp(cmd, "cd")==0)
        cd();
    if (strcmp(cmd, "pwd")==0)
        pwd();


    if (strcmp(cmd, "show")==0)
        show_dir(running->cwd);
    if (strcmp(cmd, "hits")==0)
        hit_ratio();
    if (strcmp(cmd, "exit")==0)
        quit();
    if (strcmp(cmd, "mkdir")==0)
        make_dir();
    if (strcmp(cmd, "creat") == 0)
        creat_file(pathname);
    if (strcmp(cmd, "rmdir") == 0)
        rmdir();
    if (strcmp(cmd, "link") == 0)
        link();
    if (strcmp(cmd, "unlink") == 0)
        unlink();
    if (strcmp(cmd, "symlink") == 0)
        symlink();
    if (strcmp(cmd, "open") == 0)
        open_file();
    if (strcmp(cmd, "close") == 0)
        close_file(atoi(pathname));
    if (strcmp(cmd, "pfd") == 0)
        pfd();
    if (strcmp(cmd, "read") == 0)
        read_file();
    if (strcmp(cmd, "cat") == 0)
        cat_file();
    if (strcmp(cmd, "write") == 0)
        write_file();
    if (strcmp(cmd, "cp") == 0) 
        cp_file();
    if (strcmp(cmd, "head") == 0)
        head_file();
    if (strcmp(cmd, "tail") == 0)
        tail_file();
  }
}


int show_dir(MINODE *mip)
{
    // show contents of mip DIR: same as in LAB5
    int i;
    char *cp, temp[256], sbuf[BLKSIZE];
    DIR *dp;
    printf("i_block[0] = %d\n", mip->INODE.i_block[0]);
    puts("   i_number rec_len name_len   name");
        get_block(mip->dev, mip->INODE.i_block[0], sbuf);
        dp = (DIR *)sbuf;
        cp = sbuf;
        while (cp < sbuf + BLKSIZE)
        {
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            printf("%8d%8d%8u         %s\n",
                    dp->inode, dp->rec_len, dp->name_len, temp);
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
}

int hit_ratio()
{
    // print cacheList;
    // compute and print hit_ratio
    printf("cacheList = ");
    MINODE *mip = cacheList;
    while(mip) {
        printf("c%d[%d %d]s%d->", mip->cacheCount, dev, mip->ino, mip->shareCount);
        mip = mip->next;
    }
    puts("NULL");
    printf("requests=%d hits=%d hit_ratio=%d%%\n", requests, hits, ((hits*100) / requests));
}

int quit()
{
   MINODE *mip = cacheList;
   while(mip){
     if (mip->shareCount){
        mip->shareCount = 1;
        iput(mip);    // write INODE back if modified
     }
     mip = mip->next;
   }
   exit(0);
}







