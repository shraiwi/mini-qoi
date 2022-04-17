#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mini_qoi.h"

uint64_t micros(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
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
        case MQOI_DESC_INVALID_SIZE: 
            printf("image of size %ux%u has more than 400 million pixels!\n", img_w, img_h); 
            return -1;
        default: break;
    }

    printf("image resolution: %ux%u\n", img_w, img_h);

    mqoi_dec_t dec;
    mqoi_rgba_t * px = NULL;
    FILE * opt_f = fopen(argv[2], "wb");
    uint64_t start_us, end_us;

    if (!opt_f) {
        printf("couldn't open destination image!\n");
        return -1;
    }

    mqoi_dec_init(&dec, img_w * img_h);

    start_us = micros();
    while (!mqoi_dec_done(&dec) && (read_c = fgetc(img_f)) != -1) {
        mqoi_dec_push(&dec, (char)read_c);

        while ((px = mqoi_dec_pop(&dec)) != NULL) {
            fwrite(px, 1, sizeof(mqoi_rgba_t), opt_f);
        }
    }
    end_us = micros();

    if (!mqoi_dec_done(&dec)) {
        printf("file ended before image decoding was complete!\n");
        return -1;
    }

    fclose(opt_f);
    fclose(img_f);

    printf("image decoding complete!\n");
    printf("image decoding took %lu us\n", end_us - start_us);


    return 0;    
}