#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "util.h"

#define timeOffset 14 //offset of creation time in directory entry
#define dateOffset 16 //offset of creation date in directory entry
#define bytesPerSectorOffset 11
#define sectorsPerClusterOffset 13


void print_date_time(char * directory_entry_start_pos){
	int time, date;
	int hours, minutes, day, month, year;
	
	time = *(unsigned short *)(directory_entry_start_pos + timeOffset);
	date = *(unsigned short *)(directory_entry_start_pos + dateOffset);
	
	//the year is stored as a value since 1980
	//the year is stored in the high seven bits
	year = ((date & 0xFE00) >> 9) + 1980;
	//the month is stored in the middle four bits
	month = (date & 0x1E0) >> 5;
	//the day is stored in the low five bits
	day = (date & 0x1F);
	
	printf("%d-%02d-%02d ", year, month, day);
	//the hours are stored in the high five bits
	hours = (time & 0xF800) >> 11;
	//the minutes are stored in the middle 6 bits
	minutes = (time & 0x7E0) >> 5;
	
	printf("%02d:%02d\n", hours, minutes);
	
	return;	
}

FILE * open_file(char *file_path)
{
    FILE *fp;
    fp = fopen(file_path, "r");
	if (fp == NULL){
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}
    return fp;
}

void safe_fseek(FILE *fp, int offset, int whence){
    if(fseek(fp, offset, whence)){
		perror("Error seeking");
        exit(EXIT_FAILURE);
	}
}

unsigned int bytes_per_sector(FILE * fp){
	unsigned char bytes[2];
	safe_fseek(fp, bytesPerSectorOffset, SEEK_SET);
	bytes[0] = fgetc(fp);
	bytes[1] = fgetc(fp);

	unsigned int sum = bytes[0] | (bytes[1]<<8);

	return sum;
}

unsigned int sectors_per_cluster(FILE * fp){
	unsigned char byte;
	safe_fseek(fp, sectorsPerClusterOffset, SEEK_SET);
	byte = fgetc(fp);
	return (unsigned int)byte;
}

unsigned int cluster_size(FILE * fp, unsigned int num_clusters){
	return (num_clusters * bytes_per_sector(fp) * sectors_per_cluster(fp));
}

void seek_to_sector(FILE * fp, int sector){
	unsigned int byteOffset = sector * bytes_per_sector(fp);
	safe_fseek(fp, byteOffset, SEEK_SET);
}

// Free disk space
unsigned int free_sectors(FILE * fp){
	unsigned int max_fat_size = FAT_LEN_SECTORS * bytes_per_sector(fp);
	seek_to_sector(fp, FAT_SECTOR);

	unsigned char bytes[2];
	unsigned int fat_val = 0;
	int sum = 0;
	for(int i = 0;i<max_fat_size;i+=2){
		fread(bytes, 1, 2, fp);
		fat_val = bytes[0] | (bytes[1]<<8);
		
		if(fat_val == 0){
			sum++;
		}
	}
	return sum;
}
