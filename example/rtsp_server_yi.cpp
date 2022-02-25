// RTSP Server

#include "xop/RtspServer.h"
#include "net/Timer.h"
#include "VideoFile.h"
#include "AudioFile.h"
#include "PCMSource.h"
#include "G711ASource.h"
#include "G711USource.h"
#include "DigestAuthenticator.h"
#include "tostring.h"
#include <thread>
#include <memory>
#include <iostream>
#include <string>

#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <inttypes.h>

#define RESOLUTION_NONE 0
#define RESOLUTION_LOW  360
#define RESOLUTION_HIGH 1080
#define RESOLUTION_BOTH 1440

#define AUDIO_NONE 0
#define AUDIO_PCM  1
#define AUDIO_ALAW 2
#define AUDIO_ULAW 3
#define AUDIO_AAC 4

#define DEFAULT_PORT 554

#define VIDEO_BUFFER_FILE_HIGH "/tmp/h264_high_fifo"
#define VIDEO_BUFFER_FILE_LOW  "/tmp/h264_low_fifo"
#define AUDIO_AAC_BUFFER_FILE  "/tmp/aac_audio_fifo"
#define AUDIO_PCM_BUFFER_FILE  "/tmp/audio_fifo"

struct AudioSessionId {
    xop::MediaSessionId mediaSessionId;
    xop::MediaChannelId mediaChannelId;
};

void SendVideoFrameThread(xop::RtspServer* rtsp_server, xop::MediaSessionId session_id, VideoFile* video_file);
void SendAudioFrameThread(xop::RtspServer* rtsp_server, std::vector<struct AudioSessionId> session_ids, AudioFile* audio_file, int audio_type);

void print_usage(char *progname)
{
    fprintf(stderr, "\nUsage: %s [-r RES] [-c CODEC_LOW] [-C CODEC_HIGH] [-a AUDIO] [-p PORT] [-u USER] [-w PASSWORD] [-d DEBUG]\n\n", progname);
    fprintf(stderr, "\t-r RES,   --resolution RES\n");
    fprintf(stderr, "\t\tset resolution: low, high or both (default high)\n");
    fprintf(stderr, "\t-c CODEC_LOW,   --codec_low CODEC_LOW\n");
    fprintf(stderr, "\t\tcodec of low resolution stream: h264 or h265\n");
    fprintf(stderr, "\t-C CODEC_HIGH,   --codec_high CODEC_HIGH\n");
    fprintf(stderr, "\t\tcodec of high resolution stream: h264 or h265\n");
    fprintf(stderr, "\t-a AUDIO,   --audio AUDIO\n");
    fprintf(stderr, "\t\tset audio: yes, no, alaw, ulaw, pcm or aac (default pcm)\n");
    fprintf(stderr, "\t-p PORT,   --port PORT\n");
    fprintf(stderr, "\t\tset TCP port (default 554)\n");
    fprintf(stderr, "\t-u USER,   --user USER\n");
    fprintf(stderr, "\t\tset username (default no authentication)\n");
    fprintf(stderr, "\t-w PASSWORD,   --password PASSWORD\n");
    fprintf(stderr, "\t\tset password (default no authentication)\n");
    fprintf(stderr, "\t-d DEBUG,   --debug DEBUG\n");
    fprintf(stderr, "\t\t0 none, 1 grabber, 2 rtsp library or 3 both\n");
    fprintf(stderr, "\t-h,   --help\n");
    fprintf(stderr, "\t\tprint this help\n");
}

int main(int argc, char **argv)
{
    int resolution;
    int codec_low;
    int codec_high;
    int audio;
    int port;
    std::string user;
    std::string password;
    int debug;

    VideoFile video_file_high(131072);
    VideoFile video_file_low(65536);
    AudioFile audio_file(4096);

    int c;
    char *endptr;

    resolution = RESOLUTION_HIGH;
    codec_low = CODEC_NONE;
    codec_high = CODEC_NONE;
    audio = 1;
    port = DEFAULT_PORT;

    while (1) {
        static struct option long_options[] =
        {
            {"resolution",  required_argument, 0, 'r'},
            {"codec_low",  required_argument, 0, 'c'},
            {"codec_high",  required_argument, 0, 'C'},
            {"audio",  required_argument, 0, 'a'},
            {"port",  required_argument, 0, 'p'},
            {"user",  required_argument, 0, 'u'},
            {"password",  required_argument, 0, 'w'},
            {"debug",  required_argument, 0, 'd'},
            {"help",  no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "r:c:C:a:p:u:w:d:h",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
        case 'r':
            if (strcasecmp("low", optarg) == 0) {
                resolution = RESOLUTION_LOW;
            } else if (strcasecmp("high", optarg) == 0) {
                resolution = RESOLUTION_HIGH;
            } else if (strcasecmp("both", optarg) == 0) {
                resolution = RESOLUTION_BOTH;
            }
            break;

        case 'c':
            if (strcasecmp("h264", optarg) == 0) {
                codec_low = CODEC_H264;
            } else if (strcasecmp("h265", optarg) == 0) {
                codec_low = CODEC_H265;
            }
            break;

        case 'C':
            if (strcasecmp("h264", optarg) == 0) {
                codec_high = CODEC_H264;
            } else if (strcasecmp("h265", optarg) == 0) {
                codec_high = CODEC_H265;
            }
            break;

        case 'a':
            if (strcasecmp("no", optarg) == 0) {
                audio = AUDIO_NONE;
            } else if (strcasecmp("yes", optarg) == 0) {
                audio = AUDIO_PCM;
            } else if (strcasecmp("pcm", optarg) == 0) {
                audio = AUDIO_PCM;
            } else if (strcasecmp("alaw", optarg) == 0) {
                audio = AUDIO_ALAW;
            } else if (strcasecmp("ulaw", optarg) == 0) {
                audio = AUDIO_ULAW;
            } else if (strcasecmp("aac", optarg) == 0) {
                audio = AUDIO_AAC;
            }
            break;

        case 'p':
            errno = 0;    /* To distinguish success/failure after call */
            port = strtol(optarg, &endptr, 10);

            /* Check for various possible errors */
            if ((errno == ERANGE && (port == LONG_MAX || port == LONG_MIN)) || (errno != 0 && port == 0)) {
                print_usage(argv[0]);
                return -1;
            }
            if (endptr == optarg) {
                print_usage(argv[0]);
                return -1;
            }
            break;

        case 'u':
            if (strlen(optarg) < 256) {
                user.assign(optarg);
            }
            break;

        case 'w':
            if (strlen(optarg) < 256) {
                password.assign(optarg);
            }
            break;

        case 'd':
            errno = 0;    /* To distinguish success/failure after call */
            debug = strtol(optarg, &endptr, 10);

            /* Check for various possible errors */
            if ((errno == ERANGE && (debug == LONG_MAX || debug == LONG_MIN)) || (errno != 0 && debug == 0)) {
                print_usage(argv[0]);
                return -1;
            }
            if (endptr == optarg) {
                print_usage(argv[0]);
                return -1;
            }
            if ((debug < 0) || (debug > 3)) {
                print_usage(argv[0]);
                return -1;
            }
            break;

        case 'h':
            print_usage(argv[0]);
            return -1;
            break;

        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            print_usage(argv[0]);
            return -1;
        }
    }

    printf("Resolution: %s\n", (resolution==RESOLUTION_BOTH)?"both":((resolution==RESOLUTION_HIGH)?"high":"low"));
    if (codec_low != CODEC_NONE)
        printf("Codec low: h%d\n", codec_low);
    if (codec_high != CODEC_NONE)
        printf("Codec high: h%d\n", codec_high);
    printf("Audio: %d\n", audio);
    printf("Port: %d\n", port);
    printf("User: %s\n", user.c_str());
    printf("Password: ********\n");

    if ((resolution == RESOLUTION_HIGH) || (resolution == RESOLUTION_BOTH)) {
        if(!video_file_high.Open(VIDEO_BUFFER_FILE_HIGH)) {
            printf("Open %s failed.\n", VIDEO_BUFFER_FILE_HIGH);
            return -2;
        }
    }
    if ((resolution == RESOLUTION_LOW) || (resolution == RESOLUTION_BOTH)) {
        if(!video_file_low.Open(VIDEO_BUFFER_FILE_LOW)) {
            printf("Open %s failed.\n", VIDEO_BUFFER_FILE_LOW);
            return -2;
        }
    }

    if (audio != AUDIO_NONE) {
        if (audio == AUDIO_AAC) {
            audio_file.SetType(AudioFile::AAC);
            if(!audio_file.Open(AUDIO_AAC_BUFFER_FILE)) {
                printf("Open %s failed, disable audio.\n", AUDIO_AAC_BUFFER_FILE);
                audio = AUDIO_NONE;
            }
        } else {
            audio_file.SetType(AudioFile::PCM);
            if(!audio_file.Open(AUDIO_PCM_BUFFER_FILE)) {
                printf("Open %s failed, disable audio.\n", AUDIO_PCM_BUFFER_FILE);
                audio = AUDIO_NONE;
            }
        }
    }

    std::string suffix_high = "ch0_0.h264";
    std::string suffix_low = "ch0_1.h264";
    std::string suffix_audio = "ch0_2.h264";
    std::string ip = "127.0.0.1";
    std::string sport = SSTR(port);
    std::string rtsp_url_high = "rtsp://" + ip + ":" + sport + "/" + suffix_high;
    std::string rtsp_url_low = "rtsp://" + ip + ":" + sport + "/" + suffix_low;
    std::string rtsp_url_audio = "rtsp://" + ip + ":" + sport + "/" + suffix_audio;

    std::shared_ptr<xop::EventLoop> event_loop(new xop::EventLoop());
    std::shared_ptr<xop::RtspServer> server = xop::RtspServer::Create(event_loop.get());

    if (!server->Start("0.0.0.0", atoi(sport.c_str()))) {
        printf("RTSP Server listen on %s failed.\n", sport.c_str());
        return -3;
    }

    if (user.length() > 0) {
        std::shared_ptr<xop::Authenticator> pAuthenticator = std::make_shared<xop::DigestAuthenticator>("-_-", user, password);
        server->SetAuthenticator(pAuthenticator);
    }

    xop::MediaSession *session_high;
    xop::MediaSession *session_low;
    xop::MediaSession *session_audio;
    xop::MediaSessionId session_high_id;
    xop::MediaSessionId session_low_id;
    xop::MediaSessionId session_audio_id;
    struct AudioSessionId hId;
    struct AudioSessionId lId;
    struct AudioSessionId aId;
    std::vector<struct AudioSessionId> session_ids;

    if ((resolution == RESOLUTION_HIGH) || (resolution == RESOLUTION_BOTH)) {
        session_high = xop::MediaSession::CreateNew(suffix_high);
        if (codec_high == CODEC_NONE) {
            video_file_high.DetectCodec();
            codec_high = video_file_high.GetCodec();
        }
        if (codec_high == CODEC_H264) {
            session_high->AddSource(xop::channel_0, xop::H264Source::CreateNew());
        } else if (codec_high == CODEC_H265) {
            session_high->AddSource(xop::channel_0, xop::H265Source::CreateNew());
        }
        if (audio == AUDIO_PCM) {
            xop::PCMSource* audioSource = xop::PCMSource::CreateNew();
            session_high->AddSource(xop::channel_1, audioSource);
        } else if (audio == AUDIO_ALAW) {
            xop::G711ASource* audioSource = xop::G711ASource::CreateNew();
            audioSource->SetConversion(true, 1);
            session_high->AddSource(xop::channel_1, audioSource);
        } else if (audio == AUDIO_ULAW) {
            xop::G711USource* audioSource = xop::G711USource::CreateNew();
            audioSource->SetConversion(true, 1);
            session_high->AddSource(xop::channel_1, audioSource);
        } else if (audio == AUDIO_AAC) {
            xop::AACSource* audioSource = xop::AACSource::CreateNew(16000, 1, true);
            session_high->AddSource(xop::channel_1, audioSource);
        }
        //session->StartMulticast(); 
        session_high->AddNotifyConnectedCallback([] (xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port) {
            printf("RTSP client connect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
        });

        session_high->AddNotifyDisconnectedCallback([](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port) {
            printf("RTSP client disconnect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
        });

        session_high_id = server->AddSession(session_high);
        hId.mediaSessionId = session_high_id;
        hId.mediaChannelId = xop::channel_1;
        session_ids.push_back(hId);

        std::thread t1(SendVideoFrameThread, server.get(), session_high_id, &video_file_high);
        t1.detach();

        std::cout << "Play URL: " << rtsp_url_high << std::endl;
    }
    if ((resolution == RESOLUTION_LOW) || (resolution == RESOLUTION_BOTH)) {
        session_low = xop::MediaSession::CreateNew(suffix_low);
        if (codec_low == CODEC_NONE) {
            video_file_low.DetectCodec();
            codec_low = video_file_low.GetCodec();
        }
        if (codec_low == CODEC_H264) {
            session_low->AddSource(xop::channel_0, xop::H264Source::CreateNew());
        } else if (codec_high == CODEC_H265) {
            session_low->AddSource(xop::channel_0, xop::H265Source::CreateNew());
        }
        if (audio == AUDIO_PCM) {
            xop::PCMSource* audioSource = xop::PCMSource::CreateNew();
            session_low->AddSource(xop::channel_1, audioSource);
        } else if (audio == AUDIO_ALAW) {
            xop::G711ASource* audioSource = xop::G711ASource::CreateNew();
            audioSource->SetConversion(true, 1);
            session_low->AddSource(xop::channel_1, audioSource);
        } else if (audio == AUDIO_ULAW) {
            xop::G711USource* audioSource = xop::G711USource::CreateNew();
            audioSource->SetConversion(true, 1);
            session_low->AddSource(xop::channel_1, audioSource);
        } else if (audio == AUDIO_AAC) {
            xop::AACSource* audioSource = xop::AACSource::CreateNew(16000, 1, true);
            session_low->AddSource(xop::channel_1, audioSource);
        }
        //session->StartMulticast(); 
        session_low->AddNotifyConnectedCallback([] (xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port){
            printf("RTSP client connect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
        });

        session_low->AddNotifyDisconnectedCallback([](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port) {
            printf("RTSP client disconnect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
        });

        session_low_id = server->AddSession(session_low);
        lId.mediaSessionId = session_low_id;
        lId.mediaChannelId = xop::channel_1;
        session_ids.push_back(lId);

        std::thread t1(SendVideoFrameThread, server.get(), session_low_id, &video_file_low);
        t1.detach();

        std::cout << "Play URL: " << rtsp_url_low << std::endl;
    }

    if (audio != AUDIO_NONE) {
        session_audio = xop::MediaSession::CreateNew(suffix_audio);
        if (audio == AUDIO_PCM) {
            xop::PCMSource* audioSource = xop::PCMSource::CreateNew();
            session_audio->AddSource(xop::channel_0, audioSource);
        } else if (audio == AUDIO_ALAW) {
            xop::G711ASource* audioSource = xop::G711ASource::CreateNew();
            audioSource->SetConversion(true, 1);
            session_audio->AddSource(xop::channel_0, audioSource);
        } else if (audio == AUDIO_ULAW) {
            xop::G711USource* audioSource = xop::G711USource::CreateNew();
            audioSource->SetConversion(true, 1);
            session_audio->AddSource(xop::channel_0, audioSource);
        } else if (audio == AUDIO_AAC) {
            xop::AACSource* audioSource = xop::AACSource::CreateNew(16000, 1, true);
            session_audio->AddSource(xop::channel_0, audioSource);
        }
        //session->StartMulticast(); 
        session_audio->AddNotifyConnectedCallback([] (xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port) {
            printf("RTSP client connect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
        });

        session_audio->AddNotifyDisconnectedCallback([](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port) {
            printf("RTSP client disconnect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
        });

        session_audio_id = server->AddSession(session_audio);
        aId.mediaSessionId = session_audio_id;
        aId.mediaChannelId = xop::channel_0;
        session_ids.push_back(aId);

        std::cout << "Play URL: " << rtsp_url_audio << std::endl;
    }

    if (audio != AUDIO_NONE) {
        audio_file.Reset();
        std::thread t2(SendAudioFrameThread, server.get(), session_ids, &audio_file, audio);
        t2.detach();
    }

    while (1) {
        xop::Timer::Sleep(100);
    }

    getchar();

    return 0;
}

void SendVideoFrameThread(xop::RtspServer* rtsp_server, xop::MediaSessionId session_id, VideoFile* video_file)
{
    int buf_size = 262144;
    char *p_video_buf;
    std::unique_ptr<uint8_t[]> video_buf(new uint8_t[buf_size]);

    xop::AVFrame videoFrame;
    videoFrame.type = 0;

    while(1) {
        bool end_of_frame = false;
        int video_size = video_file->ReadFrame((char *) video_buf.get(), buf_size, &end_of_frame);
        if(video_size > 0) {
            if (video_file->GetCodec() == CODEC_H264)
                videoFrame.timestamp = xop::H264Source::GetTimestamp();
            else if (video_file->GetCodec() == CODEC_H265)
                videoFrame.timestamp = xop::H265Source::GetTimestamp();
            videoFrame.buffer.assign(video_buf.get(), video_buf.get() + video_size);
            rtsp_server->PushFrame(session_id, xop::channel_0, videoFrame);
        } 
        xop::Timer::Sleep(25); // upper limit: fps is 20 -> 1000/20=50
    }
}

void SendAudioFrameThread(xop::RtspServer* rtsp_server, std::vector<struct AudioSessionId> session_ids, AudioFile* audio_file, int audio_type)
{
    int buf_size = 4096;
    std::unique_ptr<uint8_t[]> audio_buf(new uint8_t[buf_size]);

    xop::AVFrame audioFrame;
    audioFrame.type = 0;

    while(1) {
        int audio_size = audio_file->ReadFrame((char*) audio_buf.get(), buf_size);
        audioFrame.buffer.assign(audio_buf.get(), audio_buf.get() + audio_size);

        if(audio_size > 0) {
            for (auto it = begin (session_ids); it != end (session_ids); ++it) {
                if (audio_type == AUDIO_PCM) {
                    audioFrame.timestamp = xop::PCMSource::GetTimestamp(8000);
                } else if (audio_type == AUDIO_ALAW) {
                    audioFrame.timestamp = xop::G711ASource::GetTimestamp();
                } else if (audio_type == AUDIO_ULAW) {
                    audioFrame.timestamp = xop::G711USource::GetTimestamp();
                } else if (audio_type == AUDIO_AAC) {
                    audioFrame.timestamp = xop::AACSource::GetTimestamp(16000);
                }
                rtsp_server->PushFrame((*it).mediaSessionId, (*it).mediaChannelId, audioFrame);
            }
        }
        xop::Timer::Sleep(25);
    }
}
