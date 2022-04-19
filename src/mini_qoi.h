#pragma once

#ifndef MINI_QOI_H
#define MINI_QOI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define MQOI_RGB_HASH(px) ((px.r * 3 + px.g * 5 + px.b * 7) & 0b00111111)
#define MQOI_RGBA_HASH(px) ((px.r * 3 + px.g * 5 + px.b * 7 + px.a * 11) & 0b00111111)

#define MQOI_HEADER_SIZE (14)

#define MQOI_MASK_OP_2B (0b11000000)
#define MQOI_MASK_OP_8B (0b11111111)

#define MQOI_MASK_OP_LUMA_DG (0b00111111)
#define MQOI_MASK_OP_RUN (0b00111111)

// basic types

typedef enum {
    MQOI_DESC_OK = 0, // The descriptor is valid
    MQOI_DESC_INVALID_MAGIC, // The magic value isn't correct
    MQOI_DESC_INVALID_CHANNELS, // The channel number isn't valid
    MQOI_DESC_INVALID_COLORSPACE, // The colorspace isn't valid
    MQOI_DESC_INVALID_SIZE // The image exceeds the 400 million pixel soft limit
} mqoi_desc_err_t;

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
    uint8_t head;

    uint8_t magic[4];
    uint8_t width[4]; // big-endian width
    uint8_t height[4]; // big-endian height
    uint8_t channels;
    uint8_t colorspace;
} mqoi_desc_t;

// ==== chunks ====

typedef union {
    struct {
        uint8_t head;
        union {
            mqoi_rgb_t rgb;
            mqoi_rgba_t rgba;
            uint8_t drdb;
        };
    };
    uint8_t value[5];
} mqoi_chunk_t;

// ==== codecs ====

typedef struct {
    mqoi_rgba_t hashtable[64];
    mqoi_rgba_t prev_px;
    mqoi_chunk_t working_chunk;
    uint8_t working_chunk_size;
} mqoi_enc_t;

typedef struct {
    mqoi_rgba_t hashtable[64];
    mqoi_rgba_t prev_px;
    mqoi_chunk_t curr_chunk;
    uint8_t curr_chunk_head : 4;
    uint8_t curr_chunk_size : 4;
    uint32_t pix_left;
} mqoi_dec_t;

// ==== utilities ====

void mqoi_u32_write(const uint32_t * n, uint8_t * dest);
void mqoi_u32_read(const uint8_t * src, uint32_t * n);

// ==== mqoi_desc_t ====

void mqoi_desc_init(mqoi_desc_t * desc);
void mqoi_desc_push(mqoi_desc_t * desc, uint8_t byte);
uint8_t * mqoi_desc_pop(mqoi_desc_t * desc);
uint8_t mqoi_desc_verify(mqoi_desc_t * desc, uint32_t * w, uint32_t * h);
bool mqoi_desc_done(const mqoi_desc_t * desc);

/* the encoder is still WIP
void mqoi_enc_init(mqoi_enc_t * enc);
void mqoi_enc_push(mqoi_enc_t * enc, mqoi_rgba_t * pix)
mqoi_chunk_t * mqoi_enc_pop(mqoi_enc_t * enc, uint8_t * size);
*/

// ==== mqoi_dec_t ====

void mqoi_dec_init(mqoi_dec_t * dec, uint32_t n_pix);
void mqoi_dec_push(mqoi_dec_t * dec, uint8_t byte);
uint8_t mqoi_dec_take(mqoi_dec_t * dec, const uint8_t * bytes);
mqoi_rgba_t * mqoi_dec_pop(mqoi_dec_t * dec);
bool mqoi_dec_done(const mqoi_dec_t * dec); 

#ifdef __cplusplus
}
#endif

#endif
