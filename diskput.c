#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#include "util.h"

void create_FAT_entry(FILE * fp, 
                      char * file_name_and_ext, 
                      long file_create_time,
                      int cluster,
                      unsigned int file_size){

}

void write_data_region(FILE * fp, unsigned char* bytes, int sectors_needed){

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

long calc_file_size(FILE * fp){
    long fp_loc = safe_ftell(fp);
    safe_fseek(fp, 0, SEEK_END);
    long size = safe_ftell(fp);
    // Restore fp
    safe_fseek(fp, fp_loc, SEEK_SET);
    return size;
}

int main(int argc, char *argv[])
{
    long file_create_time;
    file_create_time = get_file_datetime(argv[1]);
    long file_size;

    FILE *fp = open_file(argv[1]);

    file_size = calc_file_size(fp);
    

    return EXIT_SUCCESS;
}