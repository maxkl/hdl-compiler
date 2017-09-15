
#include <stdio.h>

#include <shared/intermediate_file.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return -1;
    }

    const char *filename = argv[1];

    int ret = intermediate_file_read(filename);

    return ret;
}