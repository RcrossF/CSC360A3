#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util.h"

#define OS_NAME_OFFSET 3
#define VOLUME_LABEL_OFFSET_BOOT 43
#define NUM_SECTORS_OFFSET 19
#define SECTORS_PER_FAT 9
#define NUM_FATS 2

char * get_os_name(FILE* fp){
	safe_fseek(fp, OS_NAME_OFFSET, SEEK_SET);
	
	char * os_name = malloc(9 * sizeof(char));
	char c = fgetc(fp);
	int i = 0;
	while(c != 0){
		os_name[i] = c;
		c = fgetc(fp);
		i++;
	}

	return os_name;
}

char * get_volume_label(FILE* fp){
	char * volume_label = malloc(12 * sizeof(char));

	// First try label in boot sector
	int blank = 1;
	safe_fseek(fp, VOLUME_LABEL_OFFSET_BOOT, SEEK_SET);
	char c = fgetc(fp);
	for(int i=0;i<11;i++){
		volume_label[i] = c;
		if (c != ' ')	blank = 0;
		c = fgetc(fp);
	}

	// If label is fully blank look in root sector
	if (blank){
		// Seek to root
		seek_to_sector(fp, ROOT_SECTOR);

		// Attribute is the 11th byte from the start
		safe_fseek(fp, 11, SEEK_CUR);
		char a = fgetc(fp);
		while(a != 8){ // Stop fgetc from incrementing fp
			safe_fseek(fp, -1, SEEK_CUR); //Backtrack to start of directory entry
			safe_fseek(fp, ROOT_ITEM_SIZE, SEEK_CUR); // Jump to next entry
			a = fgetc(fp);
		}
		
		// Found our label directory
		// Backtrack from attribute field to name
		safe_fseek(fp, -12, SEEK_CUR);

		// Now read the label
		int i = 0;
		c = fgetc(fp);
		while(c != 0){
			volume_label[i] = c;
			c = fgetc(fp);
			i++;
		}
	}
	return volume_label;		
}

unsigned int get_sector_count(FILE * fp){
	unsigned char bytes[2];
	safe_fseek(fp, NUM_SECTORS_OFFSET, SEEK_SET);

	bytes[0] = fgetc(fp);
	bytes[1] = fgetc(fp);
	unsigned int sum = bytes[0] | (bytes[1]<<8);

	return sum;
}

int subdir_file_count(FILE * fp, int cluster){
	unsigned char c;
	int count = 0;
	int subdir_cluster = 0;
	unsigned char bytes;
	long fp_location = safe_ftell(fp);
	seek_to_cluster(fp, cluster);
	c = fgetc(fp);
	while(c == '.'){
		safe_fseek(fp, 31, SEEK_CUR);
		c = fgetc(fp);
	}
	// We hit the end of the files
	if(c == NULL || c == ' ')	return 0;
	
	// Count all files in directory
	while(c != NULL && c != ' '){
		// Check if we hit a subdirectory
		safe_fseek(fp, 10, SEEK_CUR); // Attribute field
		c = fgetc(fp);
		if(c == SUBDIRECTORY){
			// Get the subdir's cluster
			safe_fseek(fp, 14, SEEK_CUR);
			fread(bytes, 1, 2, fp);
			subdir_cluster = hex_to_int(bytes);
			if(subdir_cluster != 0 && subdir_cluster != 1){
				count+= subdir_file_count(fp, subdir_cluster);
			}
		}
		else{
			count++;
			safe_fseek(fp, 21, SEEK_CUR); // Seek to next name field
			c = fgetc(fp);
		}
	}
	// Restore fp to its original spot
	safe_fseek(fp, fp_location, SEEK_SET);
	return count;
}

// Returns count of all files on the disk
int count_files(FILE * fp){
	unsigned char c;
	unsigned char bytes[2];
	int cluster;
	int files = 0;
	unsigned int search_space = (ROOT_SECTOR - DATA_SECTOR) * BYTES_PER_SECTOR;
	seek_to_sector(fp, ROOT_SECTOR);
	// Advance to the extension field
	safe_fseek(fp, 8, SEEK_CUR);
	for (int i=0;i<search_space;i+=ROOT_ITEM_SIZE){
		// Check extension
		c = fgetc(fp);
		if(c != NULL && c != ' ')	files++;
		else{
			// Likely a directory
			// Check which cluster is pointed to
			safe_fseek(fp, 17, SEEK_CUR);
			fread(bytes, 1, 2, fp);
			cluster = hex_to_int(bytes);
			if(cluster != 0 && cluster != 1){
				// We have a subdirectory, follow it down
				files += subdir_file_count(fp, cluster);
			}
			// If this is the last entry attribute will be 0
			// Rewind fp to attribute field
			safe_fseek(fp, -19, SEEK_CUR);
			c = fgetc(fp);
			if (c == 0)	break;
		}
		// Set fp to the extension field for the next iteration
		seek_to_sector(fp, ROOT_SECTOR);
		safe_fseek(fp, i+ROOT_ITEM_SIZE+8, SEEK_CUR);
	}
	return files;
}

int main(int argc, char *argv[])
{
    FILE *fp = open_file_read(argv[1]);

	char * os_name = get_os_name(fp);
	char * volume_label = get_volume_label(fp);
	unsigned int total_size = get_sector_count(fp) * BYTES_PER_SECTOR;
	unsigned int empty = free_sectors(fp) * BYTES_PER_SECTOR;
	int file_count = count_files(fp);


	printf("OS Name: %s\n", os_name);
	printf("Volume Label: %s\n", volume_label);
	printf("Total size: %d B\n", total_size);
	printf("Free space: %d B\n", empty);
	printf("============\n");
	printf("File Count: %d\n", file_count);
	printf("============\n");
	printf("Number of FAT copies: %d\n", NUM_FATS); // Same for every FAT12 disk
	printf("Sectors per FAT: %d\n", SECTORS_PER_FAT); // Same for every FAT12 disk

	fclose(fp);
    return EXIT_SUCCESS;
}
