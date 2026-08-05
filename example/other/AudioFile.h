#include <iostream>
#include <string>


class AudioFile
{
public:
    AudioFile(int buf_size=8192);
    ~AudioFile();

    void SetType(int type);
    bool Open(const char *path);
    void Close();
    void Reset();

    bool IsOpened() const
    { return (m_file != NULL); }

    int ReadFrame(char* in_buf, int in_buf_size);

    static unsigned char AAC_HEADER[];
    static unsigned char AAC_HEADER_MASK[];

    static const int PCM = 0;
    static const int AAC = 1;

private:
    int ReadPCMFrame(char* in_buf, int in_buf_size);
    int ReadAACFrame(char* in_buf, int in_buf_size);

    FILE *m_file = NULL;
    char *m_buf = NULL;
    int  m_buf_size = 0;
    int  m_buf_start_index = 0;
    int  m_buf_end_index = 0;
    int  m_type;
};
