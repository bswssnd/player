#include <string.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>

typedef struct {
    AVFormatContext* demuxer;
    AVCodecContext* decoder;

    AVFrame *frame;
    AVPacket* held_pkt;

    SwrContext* resampler;
} player_t;

typedef enum {
    PLAYER_MEDIA_INAUDIBLE,
    PLAYER_MEDIA_INVALID,
    PLAYER_MEDIA_EOF
} player_err_t;

#define PLAYER_AUDIO_BUFFER_MAX_SIZE 3840

int player_init(player_t* pl);

/**
 * \brief Queue a media file specified by a URL.
 */
player_err_t player_stream(player_t* pl, const char* url);

/**
 * \brief Retrieve a ≤20ms stereo audio slice resampled to 48000 Hz, s16be.
 * \param out_data a buffer with capacity PLAYER_AUDIO_BUFFER_MAX_SIZE
 * \param out_size usable portion size of the supplied buffer
 */
player_err_t player_get_20ms_audio(player_t* pl, uint8_t* out_data, size_t *out_size);

void player_free(player_t* pl);
