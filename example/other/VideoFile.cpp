#include "VideoFile.h"
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <sys/time.h>

long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds

    return milliseconds;
}

VideoFile::VideoFile(int buf_size): m_buf_size(buf_size)
{
    m_buf = new char[m_buf_size];
    m_buf_start_index = 0;
    m_buf_end_index = 0;
}

VideoFile::~VideoFile()
{
    delete [] m_buf;
}

bool VideoFile::Open(const char *path)
{
    m_file = fopen(path, "rb");
    if(m_file == NULL) {
        return false;
    }

    if (fcntl(fileno(m_file), F_SETFL, O_NONBLOCK) != 0) {
        printf("Cannot set non-block to video fifo");
        fclose(m_file);
        return false;
    };

    return true;
}

void VideoFile::Close()
{
    if(m_file) {
        fclose(m_file);
        m_file = NULL;
    }
}

int VideoFile::ReadFrame(char* in_buf, int in_buf_size, bool* end)
{
    if(m_file == NULL) {
        return -1;
    }

    int bytes_read;
    if (m_buf_start_index > 0) {
        memmove(m_buf, &(m_buf[m_buf_start_index]), m_buf_end_index - m_buf_start_index);
        m_buf_end_index -= m_buf_start_index;
        m_buf_start_index = 0;
    }
    // The buffer is empty
    if (m_buf_end_index == 0) {
        bytes_read = (int)fread(m_buf + m_buf_end_index, 1,
                m_buf_size - m_buf_end_index, m_file);
        m_buf_end_index += bytes_read;
    }
    if (m_buf_end_index <= 5) {
        return 0;
    }

find_nalu:
    bool is_find_start = false, is_find_end = false;
    int i = 0, j = 0, start_code = 3;
    *end = false;

    for (i=0; i<m_buf_end_index-5; i++) {
        if(m_buf[i] == 0 && m_buf[i+1] == 0 && m_buf[i+2] == 1) {
            start_code = 3;
        } else if(m_buf[i] == 0 && m_buf[i+1] == 0 && m_buf[i+2] == 0 && m_buf[i+3] == 1) {
            start_code = 4;
        } else {
            continue;
        }

        if (((m_buf[i+start_code]&0x1F) == 0x7)
                || (((m_buf[i+start_code]&0x1F) == 0x1) && ((m_buf[i+start_code+1]&0x80) == 0x80))) {
            is_find_start = true;
            break;
        }
    }

    for (j = i + 4; j<m_buf_end_index-5; j++) {
        if(m_buf[j] == 0 && m_buf[j+1] == 0 && m_buf[j+2] == 1) {
            start_code = 3;
        } else if (m_buf[j] == 0 && m_buf[j+1] == 0 && m_buf[j+2] == 0 && m_buf[j+3] == 1) {
            start_code = 4;
        } else {
            continue;
        }

        if (((m_buf[j+start_code]&0x1F) == 0x7)
                || (((m_buf[j+start_code]&0x1F) == 0x1) && ((m_buf[j+start_code+1]&0x80) == 0x80))) {
            is_find_end = true;
            break;
        }
    }

    if(is_find_start && is_find_end) {
        memcpy(in_buf, &(m_buf[i]), j - i);
        m_buf_start_index = j;
    } else {
        // NALU not found, refill the buffer
        bytes_read = (int)fread(m_buf + m_buf_end_index, 1,
                m_buf_size - m_buf_end_index, m_file);
        m_buf_end_index += bytes_read;
        if (bytes_read > 0) {
            goto find_nalu;
        } else {
            return 0;
        }
    }

    return j - i;
}
