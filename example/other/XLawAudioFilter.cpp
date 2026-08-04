#include "XLawAudioFilter.h"
#include <cstdint>
#include <cstdio>

#define CLIP_ALAW 32767
#define CLIP_ULAW 32635
#define BIAS 0x84   /*!< define the add-in bias for 16 bit samples */

XLawAudioFilter::XLawAudioFilter()
{

}

XLawAudioFilter::~XLawAudioFilter()
{

}

/*
 * Convert linear pcm sample to alaw (16 bit big endian -> 8 bit)
 *
 */
uint8_t XLawAudioFilter::lin2alaw_sample(uint16_t sample)
{
    uint8_t sign;
    uint8_t exponent;
    uint8_t mantissa;
    uint8_t res;

    // get sign
    sign = (uint8_t) ((sample >> 8) & 0x80);
    if (sign != 0) sample = -sample;
    if (sample > CLIP_ALAW) sample = CLIP_ALAW;

    // get exponent
    exponent = 7;
    for (int exponent_mask = 0x4000; (sample & exponent_mask) == 0 && exponent > 0; exponent_mask = exponent_mask >> 1) {
        exponent--;
    }

    // get mantissa
    if (exponent < 2)
        mantissa = (sample >> 4) & 0x0F;
    else
        mantissa = (sample >> (exponent + 3)) & 0x0F;

    // result
    res = (sign | (exponent << 4) | mantissa);
    res = res^0xD5;

    return res;
}

/*
 * Convert linear pcm sample to ulaw (16 bit big endian -> 8 bit)
 *
 */
uint8_t XLawAudioFilter::lin2ulaw_sample(uint16_t sample)
{
    static uint8_t const exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
                                         4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                                         5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                                         5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                                         6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                                         6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                                         6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                                         6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                                         7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                                         7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                                         7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                                         7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                                         7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                                         7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                                         7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                                         7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};

    uint8_t sign;
    uint8_t exponent;
    uint8_t mantissa;
    uint8_t res;

    // get sign
    sign = (uint8_t) ((sample >> 8) & 0x80);
    if (sign != 0) sample = -sample;
    if (sample > CLIP_ULAW) sample = CLIP_ULAW;
    sample += BIAS;

    // get exponent
    exponent = exp_lut[(sample >> 7) & 0xFF];

    // get mantissa
    mantissa = (sample >> (exponent + 3)) & 0x0F;

    // result
    res = ~(sign | (exponent << 4) | mantissa);
    if (res == 0 ) res = 2; // Zero trap

    return res;
}

/*
 * Convert linear pcm frame to alaw (16 bit -> 8 bit)
 * le is the endianness: 1 if little, 0 if big
 * Reuse the same buffer to save memory
 */
int XLawAudioFilter::lin2alaw(uint8_t *frame, uint32_t size, uint32_t le)
{
    int num = size / 2; // number of samples
    int i;
    uint16_t rev;

    if (le == 1) {
        for (i = 0; i < num; i++) {
            rev = ((uint16_t) frame[i * 2]) + ((uint16_t) (frame[i * 2 + 1]) << 8);
            frame[i] = lin2alaw_sample(rev);
        }
    } else {
        for (i = 0; i < num; ++i) {
            rev =  (((uint16_t) frame[i * 2]) << 8) + (uint16_t) frame[i * 2 + 1];
            frame[i] = lin2alaw_sample(rev);
        }
    }

    return num;
}

/*
 * Convert linear pcm frame to ulaw (16 bit -> 8 bit)
 * le is the endianness: 1 if little, 0 if big
 * Reuse the same buffer to save memory
 */
int XLawAudioFilter::lin2ulaw(uint8_t *frame, uint32_t size, uint32_t le)
{
    int num = size / 2; // number of samples
    int i;
    uint16_t rev;

    if (le == 1) {
        for (i = 0; i < num; i++) {
            rev = ((uint16_t) frame[i * 2]) + ((uint16_t) (frame[i * 2 + 1]) << 8);
            frame[i] = lin2ulaw_sample(rev);
        }
    } else {
        for (i = 0; i < num; ++i) {
            rev =  (((uint16_t) frame[i * 2]) << 8) + (uint16_t) frame[i * 2 + 1];
            frame[i] = lin2ulaw_sample(rev);
        }
    }

    return num;
}
