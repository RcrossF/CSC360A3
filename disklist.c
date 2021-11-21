#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

int main(int argc, char *argv[])
{
    FILE *fp = open_file(argv[1]);
    return EXIT_SUCCESS;
}