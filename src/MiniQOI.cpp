#include "MiniQOI.h"

namespace mQOI {

    Decoder::Decoder() { mqoi_dec_init(&dec, 0); }
    Decoder::Decoder(uint32_t nPixels) { mqoi_dec_init(&dec, nPixels); }

    void Decoder::push(uint8_t byte) { mqoi_dec_push(&dec, byte); }
    uint8_t Decoder::take(const uint8_t * bytes) { return mqoi_dec_take(&dec, bytes); }
    RGBA * Decoder::pop() { return (RGBA *)mqoi_dec_pop(&dec); }
    bool Decoder::done() { return mqoi_dec_done(&dec); }

    Descriptor::Descriptor() { mqoi_desc_init(&desc); }

    void Descriptor::push(uint8_t byte) { mqoi_desc_push(&desc, byte); }
    uint8_t * Descriptor::pop() { return mqoi_desc_pop(&desc); }
    uint8_t Descriptor::verify(uint32_t * w, uint32_t * h) { return mqoi_desc_verify(&desc, w, h); }
    bool Descriptor::done() { return mqoi_desc_done(&desc); };
}