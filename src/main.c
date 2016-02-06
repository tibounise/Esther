#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
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

gdImagePtr load_image(char *image_path) {
    FILE *image_file;
    image_file = fopen(image_path,"rb");

    if (image_file == NULL) {
        return NULL;
    }

    gdImagePtr gd_image;
    gd_image = gdImageCreateFromPng(image_file);
    fclose(image_file);

    return gd_image;
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

void floyd_steinberg_dithering (int size_x, int size_y, unsigned char** bw_array) {
    for (int j = 0; j < size_y; ++j) {
        for (int i = 0; i < size_x; ++i) {
            unsigned char old_pixel = bw_array[i][j];
            
            if (bw_array[i][j] >= 0x7F) { // 0x7F = (0xFF / 2)
                bw_array[i][j] = 0xFF;
            } else {
                bw_array[i][j] = 0x00;
            }

            char delta = old_pixel-bw_array[i][j];

            if (i+1 < size_x) {
                bw_array[i+1][j] += (delta*7)/16;
            }
            if (i-1 > 0 && j+1 < size_y) {
                bw_array[i-1][j+1] += (delta*3)/16;
            }
            if (j+1 < size_y) {
                bw_array[i][j+1] += (delta*5)/16;
            }
            if (i+1 < size_x && j+1 < size_y) {
                bw_array[i+1][j+1] += delta/16;
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
    // Argument verification
    if (argc != 2) {
        fprintf(stderr, "Usage : esther [inputfile.png]\n");
        return EXIT_FAILURE;
    }

    gdImagePtr source_image;
    source_image = load_image(argv[1]);

    if (source_image == NULL) {
        fprintf(stderr, "Cannot open %s\n", argv[1]);
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

    //floyd_steinberg_dithering(image_size_x,image_size_y,bw_image_array);

    export_as_c(image_size_x,image_size_y,bw_image_array);

    free_2d_array(image_size_x,image_size_y,bw_image_array);

    return EXIT_SUCCESS;
}