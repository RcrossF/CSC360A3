#ifndef INCLUDE_H
#define INCLUDE_H
#include <stdio.h>

#define FAT_SECTOR 1
#define BYTES_PER_SECTOR 512
#define ROOT_SECTOR 19
#define DATA_SECTOR 33
#define FAT_LEN_SECTORS 9
#define ROOT_ITEM_SIZE 32
#define FAT_NUM_CLUSTERS 256 * (FAT_LEN_SECTORS+2) + 32
#define SUBDIRECTORY 16
#define LONG_ENTRY 15
#define SIZE_OFFSET 28
#define END_OF_CHAIN 4095

unsigned int hex_to_int(unsigned char *bytes);

unsigned int four_byte_hex_to_int(unsigned char *bytes);

void print_date_time(FILE * fp);

FILE * open_file_read(char *file_path);

FILE * open_file_rw(char *file_path);

void safe_fseek(FILE *fp, int offset, int whence);

unsigned int sectors_per_cluster(FILE * fp);

unsigned int cluster_size(FILE * fp, unsigned int num_clusters);

void seek_to_sector(FILE * fp, int sector);

unsigned int free_sectors(FILE * fp);

void seek_to_cluster(FILE * fp, int cluster);

int get_FAT_entry(FILE * fp, int n);

void strip_trailing_spaces(char * str, int length);

long safe_ftell(FILE *fp);

void str_to_upper(char * str);

void remove_period(char * str);

void seek_to_FAT_cluster(FILE * fp, int n);

FILE * open_file_write(char *file_path);

unsigned char * int_to_hex(int num);

void write_2_byte_int(FILE * fp, unsigned int num);

void write_4_byte_int(FILE * fp, unsigned int num);
#endif