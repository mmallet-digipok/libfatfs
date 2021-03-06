/* KallistiOS ##version##

   readwrite.c
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#include <unistd.h>

#include <dc/sd.h>
#include <kos/blockdev.h>
#include <ext2/fs_ext2.h>

#include <wchar.h>
#include <fat2fs/fs_fat.h>

int main(int argc, char *argv[]) {
    
    kos_blockdev_t sd_dev;
    uint8 partition_type;
	
	int fd;
	int num = 0;
	unsigned char buf[1024]; // Each sector is 512 bytes long

    if(sd_init()) {
        printf("Could not initialize the SD card. Please make sure that you "
               "have an SD card adapter plugged in and an SD card inserted.\n");
        exit(EXIT_FAILURE);
    }

    /* Grab the block device for the first partition on the SD card. Note that
       you must have the SD card formatted with an MBR partitioning scheme. */
    if(sd_blockdev_for_partition(0, &sd_dev, &partition_type)) {
        printf("Could not find the first partition on the SD card!\n");
        exit(EXIT_FAILURE);
    }

    /* Check to see if the MBR says that we have a FAT partition. */
    if(!fat_partition(partition_type)) {
        printf("MBR indicates a non-fat filesystem.\n");
    }
	
    /* Initialize fs_fat and attempt to mount the device. */
    if(fs_fat_init()) {
        printf("Could not initialize fs_fat!\n");
        exit(EXIT_FAILURE);
    }

    if(fs_fat_mount("/sd", &sd_dev, FS_FAT_MOUNT_READWRITE)) {
        printf("Could not mount SD card as fat2fs. Please make sure the card "
               "has been properly formatted.\n");
        exit(EXIT_FAILURE);
    }
	
	const char * text = "Hello World!!! I am text in a new file(hello.txt) in the root directory!\n";

	fd = open("/sd/hello.txt", O_RDWR | O_TRUNC | O_CREAT);
	
	printf("\n\nFile Size: %d\n", fs_total(fd));
	
	printf("Write to file\n");
	
	num = write(fd, text, strlen(text)); 
	
	printf("Number of bytes written: %d\n", num);
	
	printf("Position in file after writing %d\n", (int)fs_tell(fd));
	
	printf("New File Size: %d\n", fs_total(fd));
	
	printf("Reading the file(hello.txt) from the beginning\n");
	
	lseek(fd, 0, SEEK_SET);
	
	num = read(fd, buf, 512);
	
	printf("Number of bytes read: %d\n", num);
	
	printf("File Contents: %s\n", buf);

    close(fd);
	
    /* Clean up the filesystem and everything else */
    fs_fat_unmount("/sd");
    fs_fat_shutdown();
    sd_shutdown();

    return 0;
}


