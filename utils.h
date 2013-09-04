/* 
 * File:   utils.h
 * Author: andy
 *
 * Created on August 30, 2013, 1:55 AM
 */

#ifndef UTILS_H
#define	UTILS_H

struct node_entry;
typedef struct node_entry node_entry_t;

struct fat_long_fn_dir_entry; 
typedef struct fat_long_fn_dir_entry fat_lfn_entry_t;

struct fatfs;
typedef struct fatfs fatfs_t;

char *remove_all_chars(const char* str, char c);
void replace_all_chars(char **str, const char* replace_chars, char replace_with);
char *trimwhitespace(char *str);
int correct_filename(const char* str);
char *generate_short_filename(node_entry_t *curdir, char * fn, unsigned char attr, int *lfn);
fat_lfn_entry_t *generate_long_filename_entry(char * fn, unsigned char checksum, unsigned char order);
int write_entry(fatfs_t *fat, void * entry, unsigned char attr, int loc[]);
int *get_free_locations(fatfs_t *fat, node_entry_t *curdir, int num_entries);
unsigned char generate_checksum(char * short_filename);
int contains_lowercase(const char *str);


#endif	/* UTILS_H */
