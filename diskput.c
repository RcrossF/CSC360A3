#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#include "util.h"

long calc_file_size(char * file_name){
    FILE *fp = open_file_read(file_name);
    safe_fseek(fp, 0, SEEK_END);
    long size = safe_ftell(fp);
    fclose(fp);
    return size;
}

char * format_filename_for_FAT(char * str){
    char * new_str = malloc(12*sizeof(char));
    str_to_upper(str);
    // Space pad the name and ext
    int i = 0;
    int j = 0;
    while(i < 8){
        if(str[i] == '.'){
            for(j=i;j<8;j++){
                new_str[j] = ' ';
            }
            break;
        }
        else {
            new_str[j] = str[i];
        }
        i++;
        j++;
    }
    // Pad ext
    int ext_len;
    for(ext_len = 0;ext_len<4;ext_len++){
        if(str[i+ext_len] != NULL && str[i+ext_len] != '.'){
            new_str[j] = str[i+ext_len];
            j++;
        }
        else if(str[i+ext_len] != '.'){
            new_str[j] = ' ';
            j++;
        }
        
    }
    new_str[j] = '\0';

    return new_str;
}

unsigned char * read_file_bytes(char * file_name){
    long size = calc_file_size(file_name);
    unsigned char * bytes = malloc(size*sizeof(char));
    FILE * fp = open_file_read(file_name);
    fread(bytes, 1, size, fp);
    fclose(fp);

    return bytes;
}

void write_dir_time(FILE * fp, long file_create_time){
    struct tm * timeinfo;
    unsigned char dostime[8] = {NULL};
    unsigned char c0;
    unsigned char c1;
    unsigned char c2;
    unsigned char c3;

    timeinfo = localtime(&file_create_time);

    unsigned int hour = (timeinfo->tm_hour << 11);
    unsigned int min = (timeinfo->tm_min << 5);
    unsigned int sec = (timeinfo->tm_sec/2);
    
    unsigned int tme = hour | min | sec;
    
    write_2_byte_int(fp, tme);

    unsigned int year = ((timeinfo->tm_year - 80) << 9);
    unsigned int month =  ((1 + timeinfo->tm_mon) << 5);
    unsigned int day = timeinfo->tm_mday;
    
    unsigned int date = year | month | day;

    write_2_byte_int(fp, date);
}

// Name should be formatted like FAT expects
void create_DIR_entry(FILE * fp, 
                      int dir_offset,
                      char * file_name_and_ext, 
                      long file_create_time,
                      int cluster,
                      long file_size,
                      int file_type){
    
    // Find empty directory entry
    if(dir_offset == 0){
        seek_to_sector(fp, ROOT_SECTOR);
    }
    else{
        seek_to_cluster(fp, dir_offset);
    }
    
    while(fgetc(fp) != NULL){
        safe_fseek(fp, ROOT_ITEM_SIZE - 1, SEEK_CUR);
    }
    safe_fseek(fp, -1, SEEK_CUR);

    // Write name and ext
    fwrite(file_name_and_ext, 1, 11, fp);

    // Write attr
    fputc(file_type, fp);

    //Skip reserved and 10ms
    safe_fseek(fp, 2, SEEK_CUR);

    // Create time
    write_dir_time(fp, file_create_time);

    //Skip access
    safe_fseek(fp, 2, SEEK_CUR);

    //Skip high cluster
    safe_fseek(fp, 2, SEEK_CUR);

    // Update time
    write_dir_time(fp, file_create_time);

    // Start cluster
    write_2_byte_int(fp, cluster);

    // File size
    write_4_byte_int(fp, file_size);
}

void write_FAT_entry(FILE * fp, int cluster, int val){
    long fp_loc = safe_ftell(fp);
    unsigned char existing_bytes[2];
    unsigned char new_bytes[2];

    seek_to_FAT_cluster(fp, cluster);
    fread(existing_bytes, 1, 2, fp);
    // FAT pack
    if((cluster % 2) == 0){
        new_bytes[0] = val & 0xFF;
		new_bytes[1] = existing_bytes[1] | ((val & 0xF00) >> 8); // Keep existing lower nibble
        
    }
    else{
        new_bytes[0] = existing_bytes[0] | ((val & 0xF) << 4); // Keep existing lower nibble
        new_bytes[1] = (val & 0xFF0) >> 4;
    }

    safe_fseek(fp, -2, SEEK_CUR);
    fwrite(new_bytes, 1, 2, fp);

    // Restore fp
    safe_fseek(fp, fp_loc, SEEK_SET);
}

// Returns first cluster of new file
int write_data_region(FILE * fp, unsigned char* bytes, long file_size, int sectors_needed){
    // Check free space
    if (free_sectors(fp) < sectors_needed){
        printf("Not enough free space on disk\n");
        exit(EXIT_SUCCESS);
    }
    // Find free sector
    int sectors_written = 0;
    long bytes_written = 0;
    int current_cluster = 0;
    int last_cluster = 0;
    int first_cluster = 0;
    while(sectors_written < sectors_needed){
        if(get_FAT_entry(fp, current_cluster) == NULL){
            if (first_cluster == 0){
                first_cluster = current_cluster;
            }
            // Rewind to beginning of free sector
            safe_fseek(fp, -1, SEEK_CUR);
            // Write 512 bytes in data region
            seek_to_cluster(fp, current_cluster);
            if(file_size - bytes_written >= BYTES_PER_SECTOR){
                fwrite(&bytes[bytes_written], 1, BYTES_PER_SECTOR, fp);
                bytes_written += BYTES_PER_SECTOR;
            }
            else{
                // At the end of the file, write remaining bytes
                int needed = file_size - bytes_written;
                fwrite(&bytes[bytes_written], 1, needed, fp);
                bytes_written += needed;
            }
            // Write last cluster to FAT table
            if(last_cluster > 0){
                write_FAT_entry(fp, last_cluster, current_cluster);
            }
            last_cluster = current_cluster;
            
            sectors_written++;
            
        }
        current_cluster++;
    }
    // Write end of chain
    write_FAT_entry(fp, last_cluster, END_OF_CHAIN);

    return first_cluster;
}

long get_file_datetime(char * file_name){
    struct stat finfo;
    if (stat(file_name, &finfo)) {
        perror("Can't extract time from file");
        exit(EXIT_FAILURE);
    }
    long time = finfo.st_mtime;
    return time;
}

// Searches from fp's location for subdirectories and enters them if they exist
// Returns offset of subdir on success, 0 otherwise
int enter_subdir(FILE * fp, char * search_name){
    char name[9];
    char ext[4];
    unsigned int attr;
    unsigned char bytes[4];
    int cluster;
    while(fgetc(fp) != NULL){
        safe_fseek(fp, -1, SEEK_CUR);
        // Read name and extension
        fread(name, 1, 8, fp);
        fread(ext, 1, 3, fp);
        // Null terminate strings
        name[8] = '\0';
        ext[3] = '\0';
        // Read attribute
        attr = fgetc(fp);
        // Skip long entries
        if(attr == LONG_ENTRY){
            safe_fseek(fp, 20, SEEK_CUR);
            continue;
        }
        // Get cluster pointed to
        safe_fseek(fp, 14, SEEK_CUR);
        fread(bytes, 1, 2, fp);
        cluster = hex_to_int(bytes);

        if(name[0] == '.' || ext[0] != ' ' || attr != SUBDIRECTORY){
            safe_fseek(fp, 4, SEEK_CUR); // Seek to next entry
            continue;
        }
        else{
            // We have a subdirectory
            // Match name
            strip_trailing_spaces(name, 8);
            if(strcmp(name, search_name) == 0){
                seek_to_cluster(fp, cluster);
                return cluster;
            }
            else{
                safe_fseek(fp, 4, SEEK_CUR); // Seek to next entry
            }
        }
    }
    safe_fseek(fp, -1, SEEK_CUR);
    return 0;
}

// Checks all subdirs in string then creates a directory entry in the bottom one
void write_file_in_subdir(FILE * fp, 
                      char * subdir_str, 
                      long file_create_time,
                      int cluster,
                      long file_size,
                      int file_type,
                      char* target_name){
    seek_to_sector(fp, ROOT_SECTOR);
    if(subdir_str[0] != '/'){
        char * file_name = format_filename_for_FAT(subdir_str);
        create_DIR_entry(fp, 0, file_name, file_create_time, cluster, file_size, 0);
    }
    else{
        char* token = strtok(subdir_str, "/");
        int offset;
        while(token != NULL){
            // Check subdir existance
            str_to_upper(token);
            offset = enter_subdir(fp, token);
            if(!offset){
                printf("The directory not found\n");
                exit(EXIT_SUCCESS);
            }
            token = strtok(NULL, "/");
        }
        char * file_name = format_filename_for_FAT(target_name);
        create_DIR_entry(fp, offset, file_name, file_create_time, cluster, file_size, 0);
    }
}


int main(int argc, char *argv[])
{
    FILE *fp = open_file_rw(argv[1]);
    
    long file_create_time;
    long file_size;
    char * original_path;
    char * local_filename;
    char * token;

    memcpy(&original_path, &argv[2], strlen(argv[2])+1);

    local_filename = strtok(argv[2], "/");
    while(token != NULL){
        token = strtok(NULL, "/");
        if (token != NULL)  memcpy(&local_filename, &token, strlen(token)+1);
    }

    file_create_time = get_file_datetime(local_filename);
    
    file_size = calc_file_size(local_filename);
    
    // Write data dir first
    int sectors_needed = (file_size + 512 - 1) / 512; // Ceiling division
    int start_cluster = write_data_region(fp, read_file_bytes(local_filename), file_size, sectors_needed);

    write_file_in_subdir(fp, original_path, file_create_time, start_cluster, file_size, 0, local_filename);

    fclose(fp);
    return EXIT_SUCCESS;
}