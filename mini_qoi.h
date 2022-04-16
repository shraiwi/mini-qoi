#pragma once

#ifndef MINI_QOI_H
#define MINI_QOI_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define MQOI_RGB_HASH(px) ((px.r * 3 + px.g * 5 + px.b * 7) & 0b00111111)
#define MQOI_RGBA_HASH(px) ((px.r * 3 + px.g * 5 + px.b * 7 + px.a * 11) & 0b00111111)

#define MQOI_MASK_OP_2B (0b11000000)
#define MQOI_MASK_OP_8B (0b11111111)

#define MQOI_MASK_OP_INDEX (0b00111111)
#define MQOI_MASK_OP_LUMA_DG (0b00111111)
#define MQOI_MASK_OP_RUN (0b00111111)


// decoder working_op bits:
#define MQOI_MASK_RUN_RGBA_BIT (0b00000001)

// basic types

typedef enum {
    MQOI_OP2_INDEX   = (0b00 << 6),
    MQOI_OP2_DIFF    = (0b01 << 6),
    MQOI_OP2_LUMA    = (0b10 << 6),
    MQOI_OP2_RUN     = (0b11 << 6),
    MQOI_OP8_RUN_RGB  = (0b11111110),
    MQOI_OP8_RUN_RGBA = (0b11111111),
} mqoi_op_t;

typedef enum {
    MQOI_CHANNELS_RGB = 3,
    MQOI_CHANNELS_RGBA,
} mqoi_channels_t;

typedef enum {
    MQOI_COLORSPACE_SRGB = 0,
    MQOI_COLORSPACE_LINEAR = 1,
} mqoi_colorspace_t;

typedef union {
    struct { uint8_t r, g, b; };
    uint8_t value[3];
} mqoi_rgb_t;

typedef union {
    struct { uint8_t r, g, b, a; };
    uint8_t value[4];
} mqoi_rgba_t;

typedef struct {
    char magic[4];
    uint32_t width;
    uint32_t height;
    uint8_t channels;
    uint8_t colorspace;
} mqoi_desc_t;

// ==== chunks ====

/*typedef struct {
    uint8_t head;
    mqoi_rgb_t pix;
} mqoi_chunk_rgb_t;

typedef struct {
    uint8_t head;
    mqoi_rgba_t pix;
} mqoi_chunk_rgba_t;

typedef struct {
    uint8_t head;
} mqoi_chunk_index_t;

typedef struct {
    uint8_t head;
} mqoi_chunk_diff_t;

typedef struct {
    uint8_t head;
    uint8_t drdb;
} mqoi_chunk_luma_t;

typedef struct {
    uint8_t head;
} mqoi_chunk_run_t;*/

typedef union {
    struct {
        uint8_t head;
        union {
            mqoi_rgb_t rgb;
            mqoi_rgba_t rgba;
            uint8_t drdb;
        };
    };
    char value[5];
} mqoi_chunk_t;

// ==== codecs ====

typedef struct {
    mqoi_rgba_t hashtable[64]; // 256 bytes of memory
    mqoi_rgba_t prev_px;
    mqoi_chunk_t working_chunk;
    uint8_t working_chunk_size;
} mqoi_enc_t;

typedef struct {
    mqoi_rgba_t hashtable[64];
    mqoi_rgba_t prev_px;
    mqoi_chunk_t curr_chunk;
    uint8_t curr_chunk_pos : 4; // slightly faster
    uint8_t curr_chunk_size : 4;
    uint8_t stream_end_prog : 3;
} mqoi_dec_t;

typedef struct {
    uint8_t head;

    char magic[4];
    char width[4]; // big-endian width
    char height[4]; // big-endian height
    uint8_t channels;
    uint8_t colorspace;
} mqoi_desc_t;

void mqoi_u32_write(const uint32_t * n, char * dest);
void mqoi_u32_read(const char * src, uint32_t * n);

void mqoi_desc_init(mqoi_desc_t * desc);
void mqoi_desc_push(mqoi_desc_t * desc, char byte);
char * mqoi_desc_pop(mqoi_desc_t * desc);

// encoder is WIP
/*void mqoi_enc_init(mqoi_enc_t * enc);
void mqoi_enc_push(mqoi_enc_t * enc, mqoi_rgba_t * pix); // pushes a pixel to the encoder
mqoi_chunk_t * mqoi_enc_pop(mqoi_enc_t * enc, uint8_t * size); // returns a chunk from the encoder, returns null if invalid*/

void mqoi_dec_init(mqoi_dec_t * dec); // initialize the decoder
void mqoi_dec_push(mqoi_dec_t * dec, char byte); // pushes a byte to the decoder
mqoi_rgba_t * mqoi_dec_pop(mqoi_dec_t * dec); // returns the next pixel from the decoder, returns null if none left

#endif