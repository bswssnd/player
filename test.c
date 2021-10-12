#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "player.h"

int main() {
    player_t* pl = player_alloc();
    FILE* out;
    const char* url;
    
    url = "sine.flac";
    out = fopen("out_48000Hz_stereo.s16le", "wb");
    
    char data[PLAYER_AUDIO_BUFFER_MAX_SIZE];
    size_t data_size;

    player_init(pl);
    player_err_t err = player_stream(pl, url);

    if(err != PLAYER_NO_ERROR)
        return -1;

    while(player_get_20ms_audio(pl, data, &data_size) == PLAYER_NO_ERROR)
        fwrite(data, 1, data_size, out);
    
    player_uninit(pl);
    free(pl);
}