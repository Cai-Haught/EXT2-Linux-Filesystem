# mkdisk sh script

# sudo dd if=/dev/zero of=diskimage bs=1024 count=4096

# sudo mke2fs -b 1024 -I 128 diskimage 4096    # INODE size=128 bytes 

# sudo mount diskimage /mnt

# (cd /mnt; sudo rmdir lost+found; sudo mkdir dir1 dir2;
#                                  sudo mkdir dir1/dir3 dir2/dir4;
#                                  sudo touch file1 file2)

# (cd /mnt; sudo rmdir lost+found)
#                                 # ; sudo mkdir dir1 dir2;
#                                 #  sudo mkdir dir1/dir3 dir2/dir4;
#                                 #  sudo touch file1 file2)

# sudo umount /mnt

rm diskimage
cp 'diskimage_lvl2' diskimage

./show diskimage


rm a.out
cc -g -w main.c
./a.out
