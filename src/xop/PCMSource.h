// PHZ
// 2018-5-16

#ifndef XOP_PCM_SOURCE_H
#define XOP_PCM_SOURCE_H

#include "MediaSource.h"
#include "rtp.h"

namespace xop
{

class PCMSource : public MediaSource
{
public:
	static PCMSource* CreateNew();
	virtual ~PCMSource();

	uint32_t GetSamplerate() const
	{ return samplerate_; }

	uint32_t GetChannels() const
	{ return channels_; }

	virtual std::string GetMediaDescription(uint16_t port=0);

	virtual std::string GetAttribute();

	virtual bool HandleFrame(MediaChannelId channel_id, AVFrame frame);

	static int64_t GetTimestamp(uint32_t sampleRate);

private:
	PCMSource(uint32_t samplerate, uint32_t channels);

	uint32_t samplerate_ = 8000;
	uint32_t channels_ = 1;
};

}

#endif
