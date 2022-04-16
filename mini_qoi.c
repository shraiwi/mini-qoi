#include "mini_qoi.h"

// utils

void mqoi_u32_write(const uint32_t * n, char * dest) {
    *(dest++) = (0xff000000 & (*n)) >> 24;
	*(dest++) = (0x00ff0000 & (*n)) >> 16;
	*(dest++) = (0x0000ff00 & (*n)) >> 8;
	*(dest++) = (0x000000ff & (*n));
}

void mqoi_u32_read(const char * src, uint32_t * n) {
    *n = ((*(src++)) << 24) | 
        ((*(src++)) << 16) | 
        ((*(src++)) << 8) | 
        ((*(src++)) << 0);
}

// desc

void mqoi_desc_init(mqoi_desc_t * desc) {
    memset(desc, 0, sizeof(mqoi_desc_t));
}

// pushes a byte to the qoi image descriptor. returns true when complete
bool mqoi_desc_push(mqoi_desc_t * desc, char byte) {
    *(char *)(desc + (++desc->head)) = byte;
    return desc->head >= sizeof(mqoi_desc_t) - 1;
}

// reads a byte from the qoi image descriptor. returns null when complete.
char * mqoi_desc_pop(mqoi_desc_t * desc) {
    if (desc->head >= sizeof(mqoi_desc_t) - 1) return NULL;
    return (char *)(desc + (++desc->head));
}

// enc
/*
void mqoi_enc_init(mqoi_enc_t * enc) {
    memset(enc, 0, sizeof(mqoi_enc_t));

    enc->prev_px.a = 0xff;
}

void mqoi_enc_push(mqoi_enc_t * enc, mqoi_rgba_t * pix) {
    
}

mqoi_chunk_t * mqoi_enc_pop(mqoi_enc_t * enc, uint8_t * size) {

}*/

// dec

void mqoi_dec_init(mqoi_dec_t * dec) {
    memset(dec, 0, sizeof(mqoi_dec_t));

    dec->prev_px.a = 0xff;
}
void mqoi_dec_push(mqoi_dec_t * dec, char byte) {
    dec->curr_chunk.value[dec->curr_chunk_pos++] = byte;
    if (dec->stream_end_prog <= 6 && byte == 0 
        || dec->stream_end_prog == 7 && byte == 1)
        dec->stream_end_prog++;
}
mqoi_rgba_t * mqoi_dec_pop(mqoi_dec_t * dec) {
    if (dec->stream_end_prog == 8) return NULL; // if we're at the stream end, break.
    if (dec->curr_chunk_pos >= dec->curr_chunk_size) { // if we're at the end of the current chunk
        mqoi_rgba_t px = { 0 };
        mqoi_rgba_t * px_ptr = NULL;
        bool is_op_run = false;
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
                        px_ptr = &dec->hashtable[
                            dec->curr_chunk.head];
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
                        if (is_op_run = run) { // if there is still a run op
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
        
        if (!is_op_run) { // if the op isn't run length (i.e. it will no longer emit pixels)
            dec->curr_chunk_pos = 0;
            dec->curr_chunk_size = 0;
        }

        return px_ptr;
    }
    return NULL;
}