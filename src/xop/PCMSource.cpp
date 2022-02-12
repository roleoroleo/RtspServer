/*
 * Support for 16 bit, little endian, PCM
 */

#if defined(WIN32) || defined(_WIN32)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif
#include "PCMSource.h"
#include <cstdio>
#include <chrono>
#if defined(__linux) || defined(__linux__)
#include <sys/time.h>
#endif

using namespace xop;
using namespace std;

PCMSource::PCMSource(uint32_t samplerate, uint32_t channels)
	: samplerate_(samplerate)
	, channels_(channels)
{
	payload_    = 97;
	media_type_ = PCM;
	clock_rate_ = samplerate;
}

PCMSource* PCMSource::CreateNew()
{
    uint32_t samplerate = 8000;
    uint32_t channels = 1;
    return new PCMSource(samplerate, channels);
}

PCMSource::~PCMSource()
{
	
}

string PCMSource::GetMediaDescription(uint16_t port)
{
	char buf[100] = {0};
	sprintf(buf, "m=audio %hu RTP/AVP 97", port);
	return string(buf);
}
	
string PCMSource::GetAttribute()
{
	char buf[100] = {0};
	sprintf(buf, "a=rtpmap:97 L16/%d", clock_rate_);
	return string(buf);
}

bool PCMSource::HandleFrame(MediaChannelId channel_id, AVFrame frame)
{
	if (frame.buffer.size() > MAX_RTP_PAYLOAD_SIZE) {
		return false;
	}

	uint8_t *frame_buf  = frame.buffer.data();
	uint32_t frame_size = frame.buffer.size();

	// change endianness
	uint8_t temp;
	for (int i = 0; i < frame_size - 1; i = i + 2) {
		temp = frame_buf[i];
		frame_buf[i] = frame_buf[i + 1];
		frame_buf[i + 1] = temp;
	}

	RtpPacket rtp_pkt;
	rtp_pkt.type = frame.type;
	rtp_pkt.timestamp = frame.timestamp;
	rtp_pkt.size = frame_size + RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE;
	rtp_pkt.last = 1;

	memcpy(rtp_pkt.data.get()+RTP_TCP_HEAD_SIZE+RTP_HEADER_SIZE, frame_buf, frame_size);

	if (send_frame_callback_) {
		send_frame_callback_(channel_id, rtp_pkt);
	}

	return true;
}

int64_t PCMSource::GetTimestamp(uint32_t sampleRate)
{
	auto time_point = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now());
	return (int64_t)((time_point.time_since_epoch().count()+500) / 1000 * 4);
}
