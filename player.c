#include "player.h"

void player_init(player_t* pl) {
    pl->demuxer = NULL;
    pl->decoder = NULL;
    pl->frame = NULL;
    pl->resampler = NULL;
    pl->held_pkt = NULL;
}

int player_stream(player_t* pl, const char* url) {
    AVCodec* audcodec;
    AVStream* audio;
    AVCodecParameters* audinfo;
    int ret;

    // TODO: decide whether to implement queuing in Java or C?
    player_free(pl);

    ret = avformat_open_input(&pl->demuxer, url, NULL, NULL);
    if(ret < 0)
        return PLAYER_MEDIA_INVALID;
    
    ret = av_find_best_stream(pl->demuxer, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
    if(ret == AVERROR_STREAM_NOT_FOUND)
        return PLAYER_MEDIA_INAUDIBLE;
    else
        return PLAYER_MEDIA_INVALID;

    audio = pl->demuxer->streams[ret];
    audinfo = pl->audio->codecpar;

    pl->decoder = avcodec_alloc_context3(audcodec);
    
    swr_alloc_set_opts(pl->resampler, 
        AV_CH_LAYOUT_STEREO, 
        AV_SAMPLE_FMT_S16,
        48000,
        audinfo->channel_layout,
        audinfo->sample_fmt,
        audinfo->sample_rate,
        0, NULL);
    
    swr_init(pl->resampler);

    pl->frame = av_frame_alloc();
    
    // 48000 Hz, 16-bit (BE), Stereo, 20ms frame
    pl->frame->linesize = PLAYER_AUDIO_BUFFER_MAX_SIZE;

    // TODO: figure out if these are redundant or not
    pl->frame->channel_layout = AV_CH_LAYOUT_STEREO;
    pl->frame->sample_rate = 48000;
    pl->frame->nb_samples = 960;
    pl->frame->channels = 2;

    pl->frame->data = av_malloc(pl->frame->linesize);
}

int player_get_20ms_audio(player_t* pl, uint8_t* out_data, size_t *out_size) {
    AVPacket* pkt;
    AVFrame *in;

    if(av_read_frame(pl->demuxer, pkt) < -1)
        return PLAYER_MEDIA_EOF;

    // if packet is held before try reading that instead.
    if(pl->held_pkt == NULL)
        pl->held_pkt = pkt;

    if(avcodec_send_packet(pl->decoder, pl->held_pkt) == AVERROR(EAGAIN)) {
        pl->held_pkt = pkt;
    } else
        pl->held_pkt = NULL;

    avcodec_receive_frame(pl->decoder, in);
    av_packet_unref(pkt);

    // try to resample a frame but if suddenly the samplerate changes, reconfigure
    if(swr_convert_frame(pl->resampler, pl->frame, in) == AVERROR_INPUT_CHANGED) {
        swr_config_frame(pl->resampler, pl->frame, in);
        swr_convert_frame(pl->resampler, pl->frame, in);
    }

    // copy the frame data to the passed buffer.
    *out_size = pl->frame->linesize[0];
    memcpy(out_data, pl->frame->data[0], *out_size);

    av_frame_unref(in);
}

void player_free(player_t* pl) {
    swr_free(pl->resampler);

    av_free(pl->frame->data);
    av_frame_free(&pl->frame);
    
    avcodec_free_context(&pl->decoder);
    avformat_free_context(&pl->demuxer);
}