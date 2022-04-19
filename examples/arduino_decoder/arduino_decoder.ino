#include <Arduino.h>
#include "qoi_pgm.h"

// the c++ implementation seems to be a 2x faster on the uno, for some reason
#define USE_CPP 

#ifdef USE_CPP
#include <MiniQOI.h>
#else
#include "mini_qoi.h"
#endif

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  delay(5000);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  #ifdef USE_CPP

  mQOI::Descriptor desc;
  uint32_t imgW, imgH;
  const uint8_t * pgmHead = qoiLogoPGM;

  while (!desc.done()) {
    desc.push(qoiLogoGetNextByte(&pgmHead));
  }

  uint8_t errn = desc.verify(&imgW, &imgH);

  Serial.print("image dimensions: ");
  Serial.print(imgW);
  Serial.print("x");
  Serial.println(imgH);

  if (errn) {
    Serial.print("invalid image, code ");
    Serial.println(errn);
    digitalWrite(LED_BUILTIN, HIGH);
    loop();
  }

  uint32_t start, end, pxCount = 0;
  mQOI::Decoder dec(imgW * imgH);
  volatile mQOI::RGBA * px;

  Serial.println("starting decode...");

  digitalWrite(LED_BUILTIN, HIGH);
  start = micros();
  while (!dec.done()) {
    dec.push(qoiLogoGetNextByte(&pgmHead));

    while ((px = dec.pop()) != NULL) {
      pxCount++;
    }
  }
  end = micros();
  digitalWrite(LED_BUILTIN, LOW);

  Serial.print("decoding took ");
  Serial.print(end - start);
  Serial.print(" us for ");
  Serial.print(pxCount);
  Serial.println(" pixels");

  #else

  mqoi_desc_t desc;
  uint32_t imgW, imgH;
  const uint8_t * pgmHead = qoiLogoPGM;

  mqoi_desc_init(&desc);

  while (!mqoi_desc_done(&desc)) {
    mqoi_desc_push(&desc, qoiLogoGetNextByte(&pgmHead));
  }

  uint8_t errn = mqoi_desc_verify(&desc, &imgW, &imgH);

  Serial.print("image dimensions: ");
  Serial.print(imgW);
  Serial.print("x");
  Serial.println(imgH);

  if (errn) {
    Serial.print("invalid image, code ");
    Serial.println(errn);
    loop();
  }

  uint32_t start, end, pxCount = 0;
  mqoi_dec_t dec;
  volatile mqoi_rgba_t * px;

  mqoi_dec_init(&dec, imgW * imgH);

  Serial.println("starting decode...");

  digitalWrite(LED_BUILTIN, HIGH);
  start = micros();
  while (!mqoi_dec_done(&dec)) {
    mqoi_dec_push(&dec, qoiLogoGetNextByte(&pgmHead));

    while ((px = mqoi_dec_pop(&dec)) != NULL) {
      pxCount++;
    }
  }
  end = micros();
  digitalWrite(LED_BUILTIN, LOW);

  Serial.print("decoding took ");
  Serial.print(end - start);
  Serial.print(" us for ");
  Serial.print(pxCount);
  Serial.println(" pixels");

  #endif
}

void loop() {
  // put your main code here, to run repeatedly:
}