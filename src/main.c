#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <gd.h>
#include "main.h"

unsigned char** allocate_2d_array(int size_x, int size_y) {
    unsigned char** array = (unsigned char**) malloc(size_x*sizeof(unsigned char*));
    for (int i = 0; i < size_x; ++i) {
        array[i] = (unsigned char*) malloc(size_y*sizeof(unsigned char));
    }

    return array;
}

void free_2d_array(int size_x, int size_y, unsigned char **array) {
    for (int i = 0; i < size_y; ++i) {
        free(array[i]);
    }
    free(array);
}

bool check_image_type_supported(char *image_path) {
    return gdSupportsFileType(image_path, false);
}

gdImagePtr load_image(char *image_path) {
    return gdImageCreateFromFile(image_path);
}

unsigned char** gdimage_to_bw_array(int size_x, int size_y, gdImagePtr gd_image) {
    unsigned char** array = allocate_2d_array(size_x,size_y);
    for (int i = 0; i < size_x; ++i) {
        for (int j = 0; j < size_y; ++j) {
            int pixel = gdImageGetPixel(gd_image,i,j);
            
            int redLevel = gdImageRed(gd_image,pixel);
            int greenLevel = gdImageGreen(gd_image,pixel);
            int blueLevel = gdImageBlue(gd_image,pixel);
            
            // Following BT.601 recommendation for color -> grey conversion
            // http://www.itu.int/rec/R-REC-BT.601-7-201103-I/en
            array[i][j] = (unsigned char)((redLevel * 0.299 + greenLevel * 0.587 + blueLevel * 0.114)/3);
        }
    }

    return array;
}

unsigned char restrain_int(int input) {
    if (input < 0x00) {
        return 0x00;
    }

    if (input > 0xFF) {
        return 0xFF;
    }

    return input;
}

void floyd_steinberg_dithering (int size_x, int size_y, unsigned char** bw_array) {
    for (int j = 0; j < size_y; ++j) {
        for (int i = 0; i < size_x; ++i) {
            unsigned char old_pixel = bw_array[i][j];
            
            if (bw_array[i][j] >= 0x7F) { // 0x7F = (0xFF / 2)
                bw_array[i][j] = 0xFF;
            } else {
                bw_array[i][j] = 0x00;
            }

            int delta = old_pixel-bw_array[i][j];
            int pixbuf;

            if (i+1 < size_x) {
                pixbuf = bw_array[i+1][j] + (delta*7)/16;
                bw_array[i+1][j] = restrain_int(pixbuf);
            }
            if (i-1 > 0 && j+1 < size_y) {
                pixbuf = bw_array[i-1][j+1] + (delta*3)/16;
                bw_array[i-1][j+1] = restrain_int(pixbuf);
            }
            if (j+1 < size_y) {
                pixbuf = bw_array[i][j+1] + (delta*5)/16;
                bw_array[i][j+1] = restrain_int(pixbuf);
            }
            if (i+1 < size_x && j+1 < size_y) {
                pixbuf = bw_array[i+1][j+1] + delta/16;
                bw_array[i+1][j+1] = restrain_int(pixbuf);
            }
        }
    }
}

void export_as_c (int size_x, int size_y, unsigned char** array) {
    unsigned char number_bytes = size_x/8;
    unsigned char byte;

    printf("{\n");
    for (int y = 0; y < size_y; ++y) {
        for (int byte_count = 0; byte_count < number_bytes; ++byte_count) {
            byte = 0x00;

            for (int i = 0; i < 8; ++i) {
                if (byte_count*8+i < size_x) {
                    if (array[byte_count*8+i][y]) {
                        byte |= (0x80 >> i);
                    }
                } else {
                    break;
                }
            }

            printf("0x%02x,",byte);
        }
        printf("\n");
    }
    printf("}\n");
}

int main(int argc, char *argv[]) {
    /* Parsing options */
    char option_character;
    bool disable_dithering = false;
    bool show_help = false;

    while ((option_character = getopt(argc, argv, "dh")) != -1) {
        switch (option_character) {
        case 'd':
            disable_dithering = true;
            break;
        case 'h':
            show_help = true;
            break;
        default:
        case '?':
            fprintf(stderr, "%s: option '-%c' is invalid: ignored\n", argv[0], optopt);
            return EXIT_FAILURE;
            break;
        }
    }

    /* Stops if no image file provided */
    if (optind+1 > argc) {
        fprintf(stderr, "%s: requires an argument\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Show help */
    if (show_help) {
        printf("Usage : esther [-d] inputfile\n");
        return EXIT_SUCCESS;
    }

    if (!check_image_type_supported(argv[optind])) {
        fprintf(stderr, "Image type not supported\n");
        return EXIT_FAILURE;
    }

    gdImagePtr source_image;
    source_image = load_image(argv[optind]);

    if (source_image == NULL) {
        fprintf(stderr, "Cannot open %s\n", argv[optind]);
        return EXIT_FAILURE;
    }

    int image_size_x = gdImageSX(source_image);
    int image_size_y = gdImageSY(source_image);

    unsigned char **bw_image_array = gdimage_to_bw_array(image_size_x,image_size_y,source_image);
    gdImageDestroy(source_image);

    if (bw_image_array == NULL) {
        fprintf(stderr, "B/W conversion failed\n");
        return EXIT_FAILURE;
    }

    /* Enable dithering if needed */
    if (!disable_dithering) {
        floyd_steinberg_dithering(image_size_x,image_size_y,bw_image_array);
    }

    export_as_c(image_size_x,image_size_y,bw_image_array);

    free_2d_array(image_size_x,image_size_y,bw_image_array);

    return EXIT_SUCCESS;
}