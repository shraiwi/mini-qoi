#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>

#include "mini_qoi.h"

// uncomment to stream the file
// #define STREAM_FILE

uint64_t micros(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t us = (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
    return us;
}

int main(int argc, const char * argv[]) {

    if (argc != 3) {
        printf("usage: %s [src] [dest]\n", argv[0]);
        return -1;
    }

    mqoi_desc_t img_desc;
    uint32_t img_w, img_h;

    FILE * img_f = fopen(argv[1], "rb");
    int read_c;

    if (!img_f) {
        printf("couldn't open source image!\n");
        return -1;
    }

    mqoi_desc_init(&img_desc);

    while (!mqoi_desc_done(&img_desc) && (read_c = fgetc(img_f)) != -1) {
        mqoi_desc_push(&img_desc, (char)read_c);
    }

    if (!mqoi_desc_done(&img_desc)) {
        printf("file ended before image descriptor was complete!\n");
        return -1;
    }

    switch (mqoi_desc_verify(&img_desc, &img_w, &img_h)) {
        case MQOI_DESC_INVALID_MAGIC: printf("image had an invalid magic value!\n"); return -1;
        case MQOI_DESC_INVALID_CHANNELS: printf("image had an invalid channel number!\n"); return -1;
        case MQOI_DESC_INVALID_COLORSPACE: printf("image had an invalid colorspace!\n"); return -1;
        default: break;
    }

    printf("image resolution: %ux%u\n", img_w, img_h);

    mqoi_dec_t dec;
    mqoi_rgba_t * px = NULL;
    FILE * opt_f = fopen(argv[2], "wb");
    uint64_t start_us, end_us;

    mqoi_dec_init(&dec, img_w * img_h);

    if (!opt_f) {
        printf("couldn't open destination image!\n");
        return -1;
    }

    #ifdef STREAM_FILE

    start_us = micros();
    while (!mqoi_dec_done(&dec) && (read_c = fgetc(img_f)) != -1) {
        mqoi_dec_push(&dec, (char)read_c);

        while ((px = mqoi_dec_pop(&dec)) != NULL) {
            fwrite(px, 1, sizeof(mqoi_rgba_t), opt_f);
        }
    }
    end_us = micros();

    #else

    struct stat fstats;
    stat(argv[1], &fstats);

    size_t opt_head = 0, img_head = 0,
        img_size = fstats.st_size,
        opt_size = img_w * img_h * sizeof(mqoi_rgba_t);

    char * img_data = malloc(img_size);
    if (img_data == NULL) {
        printf("couldn't allocate input buffer!\n");
        return -1;
    }

    mqoi_rgba_t * opt_data = malloc(opt_size);
    if (opt_data == NULL) {
        printf("couldn't allocate output buffer!\n");
        return -1;
    }

    memcpy(&img_data[img_head], &img_desc.magic, MQOI_HEADER_SIZE);

    img_head += MQOI_HEADER_SIZE;

    if (fread(&img_data[img_head], 1, img_size - img_head, img_f) < img_size - img_head) {
        printf("couldn't read all compressed image data!\n");
        return -1;
    }

    start_us = micros();
    while (!mqoi_dec_done(&dec)) {
        img_head += mqoi_dec_take(&dec, &img_data[img_head]);

        while ((px = mqoi_dec_pop(&dec)) != NULL) {
            *(uint32_t *)&opt_data[opt_head++].value = *(uint32_t *)&px->value;
        }
    }
    end_us = micros();

    if (fwrite(opt_data, 1, opt_size, opt_f) < opt_size) {
        printf("couldn't write all decompressed image data!\n");
        return -1;
    }
    
    #endif

    if (!mqoi_dec_done(&dec)) {
        printf("file ended before image decoding was complete!\n");
        return -1;
    }

    fclose(opt_f);
    fclose(img_f);

    printf("image decoding took %lu us\n", end_us - start_us);
    printf("image decoding complete!\n");

    return 0;    
}
