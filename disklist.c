#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"


void format_tree_print(char *str, int width)
{
    char buf[width+1];
    printf("%-*s ", width, str);
}

void tree(FILE * fp, int target_cluster, char * dir_name){
    long fp_location = safe_ftell(fp);
    unsigned int dirs_to_search[FAT_NUM_CLUSTERS]; // Contains cluster of subdir to serach

    int num_dirs_to_search = 0;
    char search_dir_names[20][500];
    unsigned int search_space = (ROOT_SECTOR - DATA_SECTOR) * bytes_per_sector(fp);
    seek_to_sector(fp, ROOT_SECTOR);
    if(target_cluster > 0){
        seek_to_cluster(fp, target_cluster);
    }
    unsigned char name[9];
    unsigned char ext[4];
    unsigned int attr;
    unsigned char bytes[4];
    int cluster;
    unsigned int filesize;

    // Print dir name
    if(dir_name[0] != '\0'){
        printf("%s\n", dir_name);
    }
    else{
        printf("Root\n");
    }
    printf("==================\n");

    // We're at the first name field
	for (int i=0;i<search_space;i+=ROOT_ITEM_SIZE){
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

        // Get filesize
        fread(bytes, 1, 4, fp);
        filesize = four_byte_hex_to_int(bytes);

        // Seek to date/time location
        safe_fseek(fp, -18, SEEK_CUR);

        // Skip . files
        if(name[0] == '.'){
            // Set fp to the next cluster
            safe_fseek(fp, 18, SEEK_CUR);
            continue;
        }
        else if(ext[0] != NULL && ext[0] != ' ' && attr == 0){
            // We have a file
            strip_trailing_spaces(name, 8);

            // Print size
            printf("F ");
            char * tmp[32];
            sprintf(tmp, "%u", filesize);
            format_tree_print(tmp, 10);

            // Check for extension
            if (ext[0] == ' '){
                format_tree_print(name, 10);
            }
            else{
                sprintf(tmp, "%s.%s", name, ext);
                format_tree_print(tmp, 10);
            }

            print_date_time(fp);
            printf("\n");
        }
        else if(name[0] != NULL && name[0] != ' ' && cluster > 1 && attr == SUBDIRECTORY){
            // We have a subdirectory, append it to search after all files in the current directory have been visited
            printf("D 0          ");
            format_tree_print(name, 10);
            print_date_time(fp);
            printf("\n");

            // Setup for tracking directory name
            char subdir[500];
            char modified_name[500];
            memcpy(subdir, dir_name, 500);
            sprintf(modified_name, "/%s", name);
            strcat(subdir, modified_name);
            memcpy(search_dir_names[num_dirs_to_search], subdir, 500);

            dirs_to_search[num_dirs_to_search] = cluster;
            num_dirs_to_search++;
        }
        else if(name[0] == NULL && cluster == 0){
            // We hit the last entry
            break;
        }
        // Set fp to the next cluster
        safe_fseek(fp, 18, SEEK_CUR);

    }
    printf("\n");
    if(num_dirs_to_search > 0){
        for(int i = 0;i<num_dirs_to_search;i++){
            
            tree(fp, dirs_to_search[i], search_dir_names[i]);
        }
    }
    safe_fseek(fp, fp_location, SEEK_SET);
}

int main(int argc, char *argv[]){
    FILE *fp = open_file(argv[1]);
    char * dir_name = malloc(500*sizeof(char));
    dir_name[0] = '\0';
    tree(fp,0,dir_name);

    return EXIT_SUCCESS;
}