
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "utils.h"
#include "fat_defs.h"
#include "dir_entry.h"
#include "boot_sector.h"

static short sector_offset = -1; /* Makes sure we always have to read a sector (FAT table) first */
static unsigned char buffer[512];

/* Read the Fat table from the SD card and stores it in table(cache: 512 bytes) */
unsigned int read_fat_table_value(fatfs_t *fat, int byte_index) 
{
	short ptr_offset;
	unsigned int read_value = 0;
	
	/* Check if we have to read a new sector */
	if(sector_offset != (byte_index / fat->boot_sector.bytes_per_sector))
	{
		/* Calculate sector offset from file_alloc_tab_sec_loc */
		sector_offset = byte_index / fat->boot_sector.bytes_per_sector;
	
		/* Read new sector */
		fat->dev->read_blocks(fat->dev, fat->file_alloc_tab_sec_loc + sector_offset, 1, buffer);
	}
	
	ptr_offset = byte_index % fat->boot_sector.bytes_per_sector;
	
	memcpy(&read_value, &buffer[ptr_offset], fat->byte_offset);
	
	return read_value;
}

void write_fat_table_value(fatfs_t *fat, int byte_index, int value) 
{
	short ptr_offset;
	
	/* Check if we have to read a new sector */
	if(sector_offset != (byte_index / fat->boot_sector.bytes_per_sector))
	{
		/* Calculate sector offset from file_alloc_tab_sec_loc */
		sector_offset = byte_index / fat->boot_sector.bytes_per_sector;
	
		/* Read new sector */
		fat->dev->read_blocks(fat->dev, fat->file_alloc_tab_sec_loc + sector_offset, 1, buffer);
	}
	
	ptr_offset = byte_index % fat->boot_sector.bytes_per_sector;
	
	memcpy(&buffer[ptr_offset], &(value), fat->byte_offset);
    
    fat->dev->write_blocks(fat->dev, fat->file_alloc_tab_sec_loc + sector_offset, 1, buffer);
}

fatfs_t *fat_fs_init(const char *mp, kos_blockdev_t *bd) {
    fatfs_t *rv;
	
	/* For Fat32 */
	fat_extBS_32_t  fat32_boot_ext;

    if(bd->init(bd))
		return NULL;

    if(!(rv = (fatfs_t *)malloc(sizeof(fatfs_t)))) {
		bd->shutdown(bd);
		return NULL;
	}

	memset(rv, 0, sizeof(fatfs_t));
	rv->dev = bd;

	if(fat_read_bootsector(bd, &(rv->boot_sector))) {
		free(rv);
		bd->shutdown(bd);
		return NULL;
   	}

#ifdef FATFS_DEBUG
	fat_print_bootsector(&(rv->boot_sector));
#endif

	if(rv->boot_sector.table_size_16 > 0) /* Fat16 */
	{

#ifdef FATFS_DEBUG
		printf("FAT16 formatted card detected\n");
#endif
	
		rv->fat_type = FAT16;
		rv->byte_offset = 2;
		rv->root_cluster_num = 0; /* Not used. Set it to zero */
		rv->fsinfo_sector = 0; /* Not used. */
		rv->next_free_fat_index = 2;
		rv->table_size = rv->boot_sector.table_size_16;
		rv->root_dir_sectors_num = ((rv->boot_sector.root_entry_count * ENTRYSIZE) + (rv->boot_sector.bytes_per_sector - 1)) / rv->boot_sector.bytes_per_sector;
		rv->root_dir_sec_loc = rv->boot_sector.reserved_sector_count + (rv->boot_sector.table_count * rv->boot_sector.table_size_16); 
		rv->file_alloc_tab_sec_loc = rv->boot_sector.reserved_sector_count;
		rv->data_sec_loc = rv->root_dir_sec_loc + rv->root_dir_sectors_num;
		
		/* Makes sure we already have something useful in the FAT table cache */
		read_fat_table_value(rv, 4); /* rv->byte_offset * rv->next_free_fat_index = 4 */
	}
	else                                 /* Fat32 */
	{
	
#ifdef FATFS_DEBUG
		printf("FAT32 formatted card detected\n");
#endif
	
		memset(&fat32_boot_ext, 0, sizeof(fat_extBS_32_t));
		memcpy(&fat32_boot_ext, &(rv->boot_sector.extended_section), sizeof(fat_extBS_32_t));
		
		rv->fat_type = FAT32;
		rv->byte_offset = 4;
		rv->root_cluster_num = fat32_boot_ext.root_cluster;
		rv->fsinfo_sector = fat32_boot_ext.fat_info;
		rv->next_free_fat_index = get_fsinfo_nextfree(rv, rv->fsinfo_sector);
		rv->table_size = fat32_boot_ext.table_size_32;
		rv->root_dir_sectors_num = 0;
		rv->root_dir_sec_loc = rv->boot_sector.reserved_sector_count + (rv->boot_sector.table_count * fat32_boot_ext.table_size_32); 
		rv->file_alloc_tab_sec_loc = rv->boot_sector.reserved_sector_count;
		rv->data_sec_loc = rv->root_dir_sec_loc + rv->root_dir_sectors_num;
		
		/* Makes sure we already have something useful in the FAT table cache */
		read_fat_table_value(rv, 4*rv->root_cluster_num); /* 4 = byte_offset */
	}
	
	if(rv->boot_sector.total_sectors_16 != 0) {
		rv->data_sectors_num = rv->boot_sector.total_sectors_16 - rv->data_sec_loc;
	} else {
		rv->data_sectors_num = rv->boot_sector.total_sectors_32 - rv->data_sec_loc;
	}
	
	rv->total_clusters_num = rv->data_sectors_num/rv->boot_sector.sectors_per_cluster;
	
#ifdef FATFS_DEBUG
	printf("Number of sectors the FAT table takes up: %d\n", rv->table_size);
	printf("Root directory number of sectors: %d\n", rv->root_dir_sectors_num);
	printf("Root directory sector location: %d\n", rv->root_dir_sec_loc);
	printf("File allocation table sector location: %d\n", rv->file_alloc_tab_sec_loc);
	printf("File/folder data starts at sector: %d\n", rv->data_sec_loc);
	printf("Total number of data sectors: %d\n", rv->data_sectors_num);
	printf("Total number of clusters: %d\n", rv->total_clusters_num);
	printf("Next free cluster: %d\n\n\n", rv->next_free_fat_index);
#endif
	
	rv->mount = remove_all_chars(mp, '/'); 
	
	return rv;
}

void fat_fs_shutdown(fatfs_t *fs) {

    fs->dev->shutdown(fs->dev);

    free(fs);
}
