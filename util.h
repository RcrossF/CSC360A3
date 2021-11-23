#ifndef INCLUDE_H
#define INCLUDE_H
#include <stdio.h>

#define FAT_SECTOR 1
#define ROOT_SECTOR 19
#define DATA_SECTOR 33
#define FAT_LEN_SECTORS 9
#define ROOT_ITEM_SIZE 32
#define FAT_NUM_CLUSTERS 256 * (FAT_LEN_SECTORS+2) + 32

void print_date_time(char* directory_entry_start_pos);

FILE * open_file(char *file_path);

void safe_fseek(FILE *fp, int offset, int whence);

unsigned int bytes_per_sector(FILE * fp);

unsigned int sectors_per_cluster(FILE * fp);

unsigned int cluster_size(FILE * fp, unsigned int num_clusters);

void seek_to_root(FILE * fp);

unsigned int free_space(FILE * fp);

void seek_to_cluster(FILE * fp, int cluster);

int get_FAT_entry(FILE * fp, int n);
#endif