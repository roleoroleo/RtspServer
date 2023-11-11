#include "AudioFile.h"
#include <cstring>
#include <cstdio>
#include <fcntl.h>

//unsigned char AudioFile::AAC_HEADER[]         = {0xFF, 0xF1, 0x60, 0x40, 0x00, 0x1F, 0xFC};
//unsigned char AudioFile::AAC_HEADER_MASK[]    = {0xFF, 0xFF, 0xFF, 0xFC, 0x00, 0x1F, 0xFF};
unsigned char AudioFile::AAC_HEADER[]         = {0xFF, 0xF1};
unsigned char AudioFile::AAC_HEADER_MASK[]    = {0xFF, 0xFF};

AudioFile::AudioFile(int buf_size): m_buf_size(buf_size)
{
    m_buf = new char[m_buf_size];
    m_buf_start_index = 0;
    m_buf_end_index = 0;
    m_type = AudioFile::PCM;
}

AudioFile::~AudioFile()
{
    delete [] m_buf;
}

void AudioFile::SetType(int type)
{
    m_type = type;
}

bool AudioFile::Open(const char *path)
{
    m_file = fopen(path, "rb");
    if(m_file == NULL) {
        return false;
    }

    if (fcntl(fileno(m_file), F_SETFL, O_NONBLOCK) != 0) {
        printf("Cannot set non-block to audio fifo\n");
        fclose(m_file);
        return false;
    };

    return true;
}

void AudioFile::Close()
{
    if(m_file) {
        fclose(m_file);
        m_file = NULL;
    }
}

void AudioFile::Reset()
{
    // Clear the fifo
    int bytes_read = 1;
    while (bytes_read > 0) {
        bytes_read = (int) fread(m_buf, 1, m_buf_size, m_file);
    }
}

int AudioFile::ReadFrame(char* in_buf, int in_buf_size)
{
    if (m_type == AudioFile::PCM)
        return ReadPCMFrame(in_buf, in_buf_size);
    else if (m_type == AudioFile::AAC)
        return ReadAACFrame(in_buf, in_buf_size);
}

int AudioFile::ReadPCMFrame(char* in_buf, int in_buf_size)
{
    if(m_file == NULL) {
        return -1;
    }

    int bytes_read = (int) fread(m_buf, 1, m_buf_size, m_file);
    if (bytes_read > 0) {
        memcpy(in_buf, m_buf, bytes_read);
    }
    return bytes_read;
}

int AudioFile::ReadAACFrame(char* in_buf, int in_buf_size)
{
    if(m_file == NULL) {
        return -1;
    }

    int n = sizeof(AAC_HEADER);
    int bytes_read = 1;
    if (m_buf_start_index > 0) {
        memmove(m_buf, &(m_buf[m_buf_start_index]), m_buf_end_index - m_buf_start_index);
        m_buf_end_index -= m_buf_start_index;
        m_buf_start_index = 0;
    }
    // The buffer is empty
    if (m_buf_end_index == 0) {
        bytes_read = (int) fread(m_buf + m_buf_end_index, 1,
                m_buf_size - m_buf_end_index, m_file);
        m_buf_end_index += bytes_read;
    }
    if (m_buf_end_index <= n) {
        return 0;
    }

find_adts:
    bool is_find_start = false, is_find_end = false;
    int i = 0, j = 0, k = 0;

    for (i=0; i<m_buf_end_index-n; i++) {
        for (k = 0; k < n; k++) {
            if ((m_buf[i+k] & AAC_HEADER_MASK[k]) != AAC_HEADER[k])
                break;
        }

        if (k == n) {
            is_find_start = true;
            break;
        }
    }

    for (j = i + n; j<m_buf_end_index-n; j++) {
        for (k = 0; k < n; k++) {
            if ((m_buf[j+k] & AAC_HEADER_MASK[k]) != AAC_HEADER[k])
                break;
        }

        if (k == n) {
            is_find_end = true;
            break;
        }
    }

    if (is_find_start && is_find_end) {
        memcpy(in_buf, &(m_buf[i]), j - i);
        m_buf_start_index = j;
    } else {
        // ADTS not found, refill the buffer
        bytes_read = (int) fread(m_buf + m_buf_end_index, 1,
                m_buf_size - m_buf_end_index, m_file);
        m_buf_end_index += bytes_read;
        if (bytes_read > 0) {
            goto find_adts;
        } else {
            return 0;
        }
    }

    return j - i;
}
