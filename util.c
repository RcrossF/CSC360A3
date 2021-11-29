#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "util.h"

#define sectorsPerClusterOffset 13

// Converts 2 byte hex (little endian) to int
unsigned int hex_to_int(unsigned char *bytes){
	unsigned int sum = bytes[0] | (bytes[1]<<8);
	return sum;
}

void write_2_byte_int(FILE * fp, unsigned int num){
	unsigned char c0 = (num >> 8) & 0xFF;
	unsigned char c1 = (num & 0xFF);
	fputc(c1, fp);
	fputc(c0, fp);
}

void write_4_byte_int(FILE * fp, unsigned int num){
	unsigned char c0 = (num & 0xFF);
	unsigned char c1 = (num >> 8) & 0xFF;
	unsigned char c2 = (num >> 16) & 0xFF;
	unsigned char c3 = (num >> 24) & 0xFF;
	
	fputc(c0, fp);
	fputc(c1, fp);
	fputc(c2, fp);
	fputc(c3, fp);
}

// Converts 4 byte hex (little endian) to int
unsigned int four_byte_hex_to_int(unsigned char *bytes){
	unsigned int sum = bytes[0] | (bytes[1]<<8) | (bytes[2]<<16) | (bytes[3]<<32);
	return sum;
}

// Assumes fp is in the correct location for date/time
void print_date_time(FILE * fp){
	long fp_loc = safe_ftell(fp);
	unsigned char bytes[2];
	int time, date;
	int hours, minutes, day, month, year;
	

	fread(bytes, 1, 2, fp);
	time = hex_to_int(bytes);
	fread(bytes, 1, 2, fp);
	date = hex_to_int(bytes);
	
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
	
	printf("%02d:%02d", hours, minutes);
	
	safe_fseek(fp, fp_loc, SEEK_SET);
	return;	
}

FILE * open_file_read(char *file_path)
{
    FILE *fp;
    fp = fopen(file_path, "r");
	if (fp == NULL){
		perror("Error opening file, program is case sensitive");
		exit(EXIT_FAILURE);
	}
    return fp;
}

FILE * open_file_rw(char *file_path)
{
    FILE *fp;
    fp = fopen(file_path, "r+");
	if (fp == NULL){
		perror("Error opening file, program is case sensitive");
		exit(EXIT_FAILURE);
	}
    return fp;
}

FILE * open_file_write(char *file_path)
{
    FILE *fp;
    fp = fopen(file_path, "w");
	if (fp == NULL){
		perror("Error opening file, program is case sensitive");
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

long safe_ftell(FILE *fp){
	long fp_location = ftell(fp);
	if (fp_location == -1){
		perror("Ftell failed");
		exit(EXIT_FAILURE);
	}
	return fp_location;
}

unsigned int sectors_per_cluster(FILE * fp){
	unsigned char byte;
	safe_fseek(fp, sectorsPerClusterOffset, SEEK_SET);
	byte = fgetc(fp);
	return (unsigned int)byte;
}

unsigned int cluster_size(FILE * fp, unsigned int num_clusters){
	return (num_clusters * BYTES_PER_SECTOR * sectors_per_cluster(fp));
}

void seek_to_sector(FILE * fp, int sector){
	unsigned int byteOffset = sector * BYTES_PER_SECTOR;
	safe_fseek(fp, byteOffset, SEEK_SET);
}

void seek_to_cluster(FILE * fp, int cluster){
	unsigned int byteOffset = (DATA_SECTOR + cluster - 2) * BYTES_PER_SECTOR;
	safe_fseek(fp, byteOffset, SEEK_SET);
}

void seek_to_FAT_cluster(FILE * fp, int n){
	seek_to_sector(fp, FAT_SECTOR);
	safe_fseek(fp, (((3*n) / 2)), SEEK_CUR);
}

int get_FAT_entry(FILE * fp, int n) {
	int result;
	unsigned char bytes[2];

	seek_to_FAT_cluster(fp, n);
	fread(bytes, 1, 2, fp);
	if ((n % 2) == 0) {
		bytes[0] = bytes[0] & 0xFF;
		bytes[1] = bytes[1] & 0x0F;
		result = hex_to_int(bytes);
	} 
	else {
		bytes[0] = bytes[0] & 0xF0;
		bytes[1] = bytes[1] & 0xFF;
		result = (bytes[0] >> 4) + (bytes[1] << 4);
  	}
	return result;
}


// Free disk space
unsigned int free_sectors(FILE * fp){
	unsigned int sum = 0;
	for(unsigned int i = 0;i<=FAT_NUM_CLUSTERS;i++){
		if(get_FAT_entry(fp, i) == 0){
			sum++;
		}	
	}
	return sum;
}

void strip_trailing_spaces(char * str, int length){
	for(int i=length;i>0;i--){
		if(str[i] == ' ' && str[i-1] != ' '){
			str[i] = '\0';
			return;
		}
	}
}

void str_to_upper(char * str){
    int i = 0;
    while(str[i] != NULL){
        str[i] = toupper(str[i]);
        i++;
    }
}

void remove_period(char * str){
    int i = 0;
    while(str[i] != NULL){
        if (str[i]=='.'){ 
            for (int j=i; j<strlen(str); j++){
                str[j]=str[j+1];
            } 
        }
        else {
            i++;
        }
    }
}