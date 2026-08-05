#include <iostream>
#include <string>

class XLawAudioFilter
{
public:
    XLawAudioFilter();
    ~XLawAudioFilter();

    static int lin2alaw(uint8_t *frame, uint32_t size, uint32_t le);
    static int lin2ulaw(uint8_t *frame, uint32_t size, uint32_t le);

private:
    static uint8_t lin2alaw_sample(uint16_t sample);
    static uint8_t lin2ulaw_sample(uint16_t sample);
};
