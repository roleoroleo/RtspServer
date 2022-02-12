#include <iostream>
#include <string>

class VideoFile
{
public:
    VideoFile(int buf_size=262144);
    ~VideoFile();

    bool Open(const char *path);
    void Close();

    bool IsOpened() const
    { return (m_file != NULL); }

    int ReadFrame(char* in_buf, int in_buf_size, bool* end);

private:
    FILE *m_file = NULL;
    char *m_buf = NULL;
    int  m_buf_size = 0;
    int  m_buf_start_index = 0;
    int  m_buf_end_index = 0;
};
