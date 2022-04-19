// This is the Arduino library version of mQOI

#include "stdint.h"
#include <stddef.h>

namespace mQOI {

#include "mini_qoi.h"

  typedef mqoi_rgba_t RGBA;
  typedef mqoi_rgb_t RGB;

  class Decoder {
    public:
      Decoder();
      Decoder(uint32_t nPixels);

      void push(uint8_t byte);
      uint8_t take(const uint8_t * bytes);
      RGBA * pop();
      bool done();
    
    private:
      mqoi_dec_t dec;
  };

  class Descriptor {
    public:
      Descriptor();

      void push(uint8_t byte);
      uint8_t * pop();
      uint8_t verify(uint32_t * w, uint32_t * h);
      bool done();

      uint8_t& channels() { return desc.channels; }
      const uint8_t& channels() const { return desc.channels; }
      
      uint8_t& colorspace() { return desc.colorspace; }
      const uint8_t& colorspace() const { return desc.colorspace; }
      
    private:
      mqoi_desc_t desc;
  };
}
