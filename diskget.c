#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

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
        // Null terminate string
        name[12] = '\0';
        // Read attribute
        attr = fgetc(fp);

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
            fread(chunk, 1, size - bytes_so_far, fp);
            bytes_so_far += (size - bytes_so_far);
        }
        else{
            fread(chunk, 1, 512, fp);
            bytes_so_far += 512;
        }
        strcat(file_bytes, chunk);

        // Find next data cluster to read
        cluster = get_FAT_entry(fp, cluster);
    }
    // TODO: Un null-terminate the file
    return file_bytes;
}

int main(int argc, char *argv[])
{
    char search_file[50];
    int dir_entry;
    unsigned int size;

    FILE *fp = open_file(argv[1]);
    strcat(search_file, argv[2]);

    str_to_upper(search_file);
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


    // Write file to current linux dir
    return EXIT_SUCCESS;
}