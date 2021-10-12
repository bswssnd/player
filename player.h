#include <stddef.h>
#include <stdint.h>

#ifndef BITTU_PLAYER
#define BITTU_PLAYER
typedef struct player player_t;

typedef enum {
    PLAYER_NO_ERROR,
    PLAYER_MEDIA_INAUDIBLE,
    PLAYER_MEDIA_INVALID,
    PLAYER_MEDIA_DECODE_ERROR,
    PLAYER_MEDIA_INTERNAL_ERROR,
    PLAYER_MEDIA_EOF
} player_err_t;

#define PLAYER_AUDIO_BUFFER_MAX_SIZE 3840 // 960, stereo, 16-bit

player_t* player_alloc();

player_err_t player_init(player_t* pl);

/**
 * \brief Queue a media file specified by a URL.
 */
player_err_t player_stream(player_t* pl, const char* url);

/**
 * \brief Retrieve a ≤20ms stereo audio slice resampled to 48000 Hz, s16be.
 * \param out_data a buffer with capacity PLAYER_AUDIO_BUFFER_MAX_SIZE
 * \param out_size usable portion size (in bytes) of the supplied buffer
 */
player_err_t player_get_20ms_audio(player_t* pl, uint8_t* out_data, size_t *out_size);

void player_uninit(player_t* pl);
#endif