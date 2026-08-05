#include <iostream>
#include <string>

#define CODEC_NONE      0
#define CODEC_H264      264
#define CODEC_H265      265

class VideoFile
{
public:
    VideoFile(int buf_size=262144);
    ~VideoFile();

    int GetCodec();
    void SetCodec(int codec);
    bool Open(const char *path);
    void Close();

    bool IsOpened() const
    { return (m_file != NULL); }

    void DetectCodec();
    int ReadFrame(char* in_buf, int in_buf_size, bool* end);

private:
    int ReadFrameH264(char* in_buf, int in_buf_size, bool* end);
    int ReadFrameH265(char* in_buf, int in_buf_size, bool* end);

    FILE *m_file = NULL;
    char *m_buf = NULL;
    int  m_buf_size = 0;
    int  m_codec = CODEC_H264;
    int  m_buf_start_index = 0;
    int  m_buf_end_index = 0;
};
