#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util.h"

#define OS_NAME_OFFSET 3
#define VOLUME_LABEL_OFFSET_BOOT 43
#define BYTES_PER_FAT_DIRECTORY_ENTRY 32
#define NUM_SECTORS_OFFSET 19

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
			safe_fseek(fp, BYTES_PER_FAT_DIRECTORY_ENTRY, SEEK_CUR); // Jump to next entry
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

int main(int argc, char *argv[])
{
    FILE *fp = open_file(argv[1]);

	char * os_name = get_os_name(fp);
	char * volume_label = get_volume_label(fp);
	unsigned int total_size = get_sector_count(fp) * bytes_per_sector(fp);

	printf("OS Name: %s\n", os_name);
	printf("Volume Label: %s\n", volume_label);
	printf("Total size: %d B\n", total_size);

    return EXIT_SUCCESS;
}
