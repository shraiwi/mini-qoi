#include "mini_qoi.h"

#define MQOI_ISHOSTLE (*(uint8_t *)&(__mqoi_le) == 1)

#ifdef _MSC_VER
#include <stdlib.h>
#define MQOI_BSWAP32 _byteswap_ulong
#define MQOI_INLINE
#else
#define MQOI_BSWAP32 __builtin_bswap32
#define MQOI_INLINE inline
#endif

static const uint16_t __mqoi_le = 1;

// ==== utilities ====

/*
Writes a big-endian unsigned 32-bit integer from n to dest.
*/
void mqoi_u32_write(const uint32_t * n, uint8_t * dest) {
    if (MQOI_ISHOSTLE) {
        *(uint32_t *)dest = MQOI_BSWAP32(*n); // swap bytes if little endian
    } else {
        *(uint32_t *)dest = *n;
    }
}

/*
Reads a big-endian unsigned 32-bit integer from src into n.
*/
void mqoi_u32_read(const uint8_t * src, uint32_t * n) {
    if (MQOI_ISHOSTLE) {
        *n = MQOI_BSWAP32(*(uint32_t *)src); // swap bytes if little endian
    } else {
        *n = *(uint32_t *)src;
    }
}

// ==== mqoi_desc_t ====

/*
Initializes an mQOI image descriptor object.
*/
void mqoi_desc_init(mqoi_desc_t * desc) {
    memset(desc, 0, sizeof(mqoi_desc_t));
}

/* 
Pushes a byte to the mQOI image descriptor object.
*/
void mqoi_desc_push(mqoi_desc_t * desc, uint8_t byte) {
    ((uint8_t *)desc)[++desc->head] = byte;
}

/*
Reads a byte from the mQOI image descriptor object.
Returns NULL when there are none left to read.
*/
uint8_t * mqoi_desc_pop(mqoi_desc_t * desc) {
    if (desc->head >= sizeof(mqoi_desc_t) - 1) return NULL;
    return (uint8_t *)(desc + (++desc->head));
}

/*
Checks if a mQOI image descriptor is valid, and reads out its width and height into w and h.
If it returns a nonzero code, it is invalid (use mqoi_desc_err_t to understand what it means)
*/
uint8_t mqoi_desc_verify(mqoi_desc_t * desc, uint32_t * w, uint32_t * h) {
    if (desc->magic[0] != 'q' || desc->magic[1] != 'o' 
        || desc->magic[2] != 'i' || desc->magic[3] != 'f') {
        return MQOI_DESC_INVALID_MAGIC;
    }

    mqoi_u32_read(desc->width, w);
    mqoi_u32_read(desc->height, h);

    if (desc->channels != MQOI_CHANNELS_RGB && desc->channels != MQOI_CHANNELS_RGBA) {
        return MQOI_DESC_INVALID_CHANNELS;
    }

    if (desc->colorspace != MQOI_COLORSPACE_SRGB && desc->colorspace != MQOI_COLORSPACE_LINEAR) {
        return MQOI_DESC_INVALID_COLORSPACE;
    }

    return MQOI_DESC_OK;
}

/*
Returns true when the mQOI image descriptor object is completely populated.
*/
MQOI_INLINE bool mqoi_desc_done(const mqoi_desc_t * desc) {
    return desc->head >= sizeof(mqoi_desc_t) - 1;
}

// ==== mqoi_dec_t ====

/*
Initializes an mQOI decoder object. 
If number of pixels in the image are given (via n_pix), the mqoi_dec_done function will work.
*/
void mqoi_dec_init(mqoi_dec_t * dec, uint32_t n_pix) {
    memset(dec, 0, sizeof(mqoi_dec_t));

    dec->prev_px.a = 0xff;
    dec->pix_left = n_pix;
}

/*
Pushes a byte to the mQOI decoder.
Don't call this more than once unless all the pixels have been popped via mqoi_dec_pop!
Also, please don't intermingle this function with calls to mqoi_dec_take!
*/
void mqoi_dec_push(mqoi_dec_t * dec, uint8_t byte) {
    dec->curr_chunk.value[dec->curr_chunk_head++] = byte;

    if (dec->curr_chunk_size == 0 && dec->curr_chunk_head == 1) { // if we have the head of a new chunk, get its size
        
        switch (dec->curr_chunk.head) { // test 8-bit tags
            case MQOI_OP8_RUN_RGBA: dec->curr_chunk_size = 5; break;
            case MQOI_OP8_RUN_RGB: dec->curr_chunk_size = 4; break;
            default: { // test 2-bit tags
                // chunk size is 1 unless it's an OP_LUMA chunk
                dec->curr_chunk_size = 1 + ((dec->curr_chunk.head & MQOI_MASK_OP_2B) == MQOI_OP2_LUMA);
                break;
            }
        }
    }
}

/*
Automatically read up to 5 bytes so that a subsequent call of mqoi_dec_pop will not return NULL.
Returns the number of bytes that have been "taken" from the bytes array.
Please don't intermingle this function with calls to mqoi_dec_push!
*/
uint8_t mqoi_dec_take(mqoi_dec_t * dec, const uint8_t * bytes) {

    dec->curr_chunk.value[dec->curr_chunk_head++] = *(bytes++);

    switch (dec->curr_chunk.head) { // test 8-bit tags
        case MQOI_OP8_RUN_RGBA: dec->curr_chunk_size = 5; break;
        case MQOI_OP8_RUN_RGB: dec->curr_chunk_size = 4; break;
        default: { // test 2-bit tags
            // chunk size is 1 unless it's an OP_LUMA chunk
            dec->curr_chunk_size = 1 + ((dec->curr_chunk.head & MQOI_MASK_OP_2B) == MQOI_OP2_LUMA);
            break;
        }
    }

    while (dec->curr_chunk_head < dec->curr_chunk_size) {
        dec->curr_chunk.value[dec->curr_chunk_head++] = *(bytes++);
    }

    return dec->curr_chunk_head;
}


/*
Pops a pixel from the mQOI decoder.
Returns NULL if more data is needed, otherwise it returns the address of the next pixel.
*/
mqoi_rgba_t * mqoi_dec_pop(mqoi_dec_t * dec) {
    
    if (dec->curr_chunk_size && dec->curr_chunk_head >= dec->curr_chunk_size) { // if we're at the end of the current chunk
        mqoi_rgba_t px = { .a = dec->prev_px.a };
        mqoi_rgba_t * px_ptr = NULL;
        bool chunk_done = true;
        switch (dec->curr_chunk.head) { // test 8-bit tags
            case MQOI_OP8_RUN_RGBA: // rgba handled
                px.a = dec->curr_chunk.rgba.a;
            case MQOI_OP8_RUN_RGB: // rgb handled
                px.r = dec->curr_chunk.rgb.r;
                px.g = dec->curr_chunk.rgb.g;
                px.b = dec->curr_chunk.rgb.b;
                break;
            default: { // test 2-bit tags
                switch (dec->curr_chunk.head & MQOI_MASK_OP_2B) {
                    case MQOI_OP2_INDEX:
                        px_ptr = &dec->hashtable[dec->curr_chunk.head]; // no need to mask bits because the top bits are zero
                        break;
                    case MQOI_OP2_DIFF:
                        // shift out each channel and compute difference
                        px.b = dec->prev_px.b - 2 + (dec->curr_chunk.head & 0b11); // read out db
                        dec->curr_chunk.head >>= 2; // shift in dg
                        px.g = dec->prev_px.g - 2 + (dec->curr_chunk.head & 0b11); // read out dg
                        dec->curr_chunk.head >>= 2; // shift in dr
                        px.r = dec->prev_px.r - 2 + (dec->curr_chunk.head & 0b11); // read out dr
                        break;
                    case MQOI_OP2_LUMA: {
                        int8_t dg = (dec->curr_chunk.head & MQOI_MASK_OP_LUMA_DG) - 32;

                        px.g = dec->prev_px.g + dg;

                        px.b = dec->prev_px.b + (dec->curr_chunk.drdb & 0b1111) - 8 + dg;
                        dec->curr_chunk.drdb >>= 4;
                        px.r = dec->prev_px.r + (dec->curr_chunk.drdb & 0b1111) - 8 + dg;

                        break;
                    }
                    case MQOI_OP2_RUN: {
                        uint8_t run = dec->curr_chunk.head & MQOI_MASK_OP_RUN;
                        px_ptr = &dec->prev_px;
                        if (run) { // if there are still pixels left to emit,
                            chunk_done = false; // don't reset the chunk (bring us back to the block)
                            dec->curr_chunk.head--;
                        }
                        break;
                    }
                }
                break;
            }
                
        }

        if (px_ptr == NULL) { // if the return pixel is null (not in hashtable yet)
            uint8_t px_hash = MQOI_RGBA_HASH(px); // hash it
            px_ptr = &dec->hashtable[px_hash]; // get its hashed position

            px_ptr->r = px.r;
            px_ptr->g = px.g;
            px_ptr->b = px.b;
            px_ptr->a = px.a;
        }
        
        if (chunk_done) { // if the chunk is done, reset the head
            dec->curr_chunk_head = 0;
            dec->curr_chunk_size = 0;
        }

        dec->prev_px.r = px_ptr->r;
        dec->prev_px.g = px_ptr->g;
        dec->prev_px.b = px_ptr->b;
        dec->prev_px.a = px_ptr->a;

        dec->pix_left--;

        return px_ptr;
    }

    return NULL;
}

/* 
Returns true if the decoder has emitted all pixels necessary to complete the image being decoded.
Note that this function will only work if n_pix was given during the initialization of the decoder.
*/
MQOI_INLINE bool mqoi_dec_done(const mqoi_dec_t * dec) {
    return dec->pix_left == 0;
}
