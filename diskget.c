#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"


void write_to_linux(char * file_name, unsigned char* data, unsigned int size){
    FILE * fp = open_file_write(file_name);
    if (fp == NULL){
        perror("Error opening file to write");
        exit(EXIT_FAILURE);
    }

    fwrite(data, size, sizeof(char), fp);

    fclose(fp);
}

// Returns filesize of directory entry fp currently points to
unsigned int file_size(FILE * fp){
    unsigned char bytes[4];
    long fp_loc = safe_ftell(fp);
    safe_fseek(fp, SIZE_OFFSET, SEEK_CUR);
    fread(bytes, 1, 4, fp);
    safe_fseek(fp, fp_loc, SEEK_SET);
    return four_byte_hex_to_int(bytes);
}

// Returns address of directory entry of matching file, 0 on failure
int find_file(FILE * fp, char* search_name){
    unsigned char name[12];
    unsigned char ext[4];
    unsigned int attr;

    seek_to_sector(fp, ROOT_SECTOR);
    unsigned int search_space = (ROOT_SECTOR - DATA_SECTOR) * BYTES_PER_SECTOR;
    for (unsigned int i=0;i<search_space;i+=ROOT_ITEM_SIZE){
        // Read name and extension
        //TODO: strip trailing name spaces or change how strings are compared
        fread(name, 1, 8, fp);
        fread(ext, 1, 3, fp);
        // Null terminate strings
        strip_trailing_spaces(name, 8);
        ext[3] = '\0';
        // Read attribute
        attr = fgetc(fp);

        // Join extension to name for easier comparison with filename
        strcat(name, ext);
        //Match name, extension, and attribute (dir/volumeID could have same name)
        if((strcmp(name, search_name) == 0) && attr == 0){
            // Rewind fp to start of directory entry
            safe_fseek(fp, -12, SEEK_CUR);
            return safe_ftell(fp);
        }
        else if(name[0] == NULL && attr == 0){
            // Last entry
            break;
        }
    }
    return 0;
}

unsigned char * read_file(FILE * fp, int start_dir_entry, unsigned int size){
    unsigned char * file_bytes = malloc(size*sizeof(char) + 1);
    unsigned char * chunk[513];
    int chunk_size;
    unsigned char bytes[2];
    int cluster;
    unsigned int bytes_so_far = 0;

    chunk[512] = '\0';

    //Get first cluster
    safe_fseek(fp, 26, SEEK_CUR);
    fread(bytes, 1, 2, fp);
    cluster = hex_to_int(bytes);

    while(bytes_so_far != size){
        // Read data cluster
        seek_to_sector(fp, cluster + DATA_SECTOR - 2);
        if(size - bytes_so_far < 512){
            chunk_size = size - bytes_so_far;
        }  
        else{
            chunk_size = 512;
        }
        fread(chunk, 1, chunk_size, fp);
        memcpy(&file_bytes[bytes_so_far], chunk, chunk_size);
        bytes_so_far += chunk_size;

        // Find next data cluster to read
        cluster = get_FAT_entry(fp, cluster);
    }

    return file_bytes;
}

int main(int argc, char *argv[])
{
    char search_file[50] = {NULL};
    char new_file[50] = {NULL};
    int dir_entry;
    unsigned int size;

    FILE *fp = open_file_read(argv[1]);
    strcat(search_file, argv[2]);

    str_to_upper(search_file);
    strcat(new_file, search_file);
    remove_period(search_file);

    // Find file's directory entry
    dir_entry = find_file(fp, search_file);
    if(dir_entry == 0){
        printf("File not found.\n");
        exit(EXIT_SUCCESS);
    }

    // Find out how much space to allocate for the file
    safe_fseek(fp, dir_entry, SEEK_SET);
    size = file_size(fp);

    // Read file into memory
    unsigned char * file_bytes = read_file(fp, dir_entry, size);

    // Write file to current linux dir
    write_to_linux(new_file, file_bytes, size);

    fclose(fp);
    return EXIT_SUCCESS;

}