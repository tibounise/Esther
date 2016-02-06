#include <stdbool.h>
#include <gd.h>
#include "main.h"

gdImagePtr read_image(char *image_path) {
    FILE *source_image_file;
    source_image_file = fopen(image_path);

    if (source_image_file == NULL) {
        return NULL;
    }



    return;
}

int main(int argc, char *argv[]) {
    // Argument verification
    if (argc != 2) {
        fprintf(stderr, "Usage : esther [png file]\n");
        return EXIT_FAILURE;
    }

    if (read_image(argv[1]) == NULL) {
        fprintf(stderr, "Couldn't open %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    return 0;
}