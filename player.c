#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>

#include "player.h"

typedef struct player {
    AVFormatContext* demuxer;
    AVCodecContext* decoder;

    AVFrame *out_frame, *in_frame;
    AVPacket* pkt;
    AVStream* stream;

    bool pkt_held;
    bool fmt_eof;
    bool codec_eof;

    SwrContext* resampler;
} player_t;

player_t* player_alloc() {
    return malloc(sizeof(player_t));
}

player_err_t player_init(player_t* pl) {
    pl->demuxer = NULL;
    pl->decoder = NULL;
    pl->out_frame = NULL;
    pl->resampler = NULL;

    pl->pkt_held = false;
    pl->fmt_eof = false;
    pl->codec_eof = false;

    pl->in_frame = av_frame_alloc();
    pl->out_frame = av_frame_alloc();

    if(!pl->in_frame || !pl->out_frame)
        return PLAYER_MEDIA_INTERNAL_ERROR;
    
    // 48000 Hz, 16-bit (BE), Stereo, 20ms out_frame
    pl->out_frame->format = AV_SAMPLE_FMT_S16;
    pl->out_frame->channel_layout = AV_CH_LAYOUT_STEREO;
    pl->out_frame->sample_rate = 48000;
    pl->out_frame->nb_samples = 960;

    if(av_frame_get_buffer(pl->out_frame, 0))
        return PLAYER_MEDIA_INTERNAL_ERROR;

    pl->pkt = av_packet_alloc();
    if(pl->pkt)
        return PLAYER_MEDIA_INTERNAL_ERROR;
}

player_err_t player_stream(player_t* pl, const char* url) {
    AVCodec* audcodec;
    AVStream* audio;
    AVCodecParameters* audinfo;
    int ret;

    if(avformat_open_input(&pl->demuxer, url, NULL, NULL) < 0)
        return PLAYER_MEDIA_INVALID;

    if(avformat_find_stream_info(pl->demuxer, NULL) < 0)
        return PLAYER_MEDIA_INVALID;
    
    ret = av_find_best_stream(pl->demuxer, AVMEDIA_TYPE_AUDIO, -1, -1, &audcodec, 0);
    if(ret == AVERROR_STREAM_NOT_FOUND)
        return PLAYER_MEDIA_INAUDIBLE;
    else if(ret < 0)
        return PLAYER_MEDIA_INVALID;

    pl->stream = pl->demuxer->streams[ret];
    audinfo = pl->stream->codecpar;

    pl->decoder = avcodec_alloc_context3(audcodec);
    if(!pl->decoder)
        return PLAYER_MEDIA_INVALID;

    if(avcodec_parameters_to_context(pl->decoder, pl->stream->codecpar) < 0)
        return PLAYER_MEDIA_INVALID;
    
    if(avcodec_open2(pl->decoder, audcodec, NULL) < 0)
        return PLAYER_MEDIA_INVALID;
    
    pl->resampler = swr_alloc_set_opts(NULL,
        AV_CH_LAYOUT_STEREO, 
        AV_SAMPLE_FMT_S16,
        48000,
        audinfo->channel_layout,
        audinfo->format,
        audinfo->sample_rate,
        0, NULL);

    if(pl->resampler == NULL)
        return PLAYER_MEDIA_INTERNAL_ERROR;
    
    // aggressive options for throughput over unnoticable quality decrease
    av_opt_set_int(pl->resampler, "dither_method", SWR_DITHER_NONE, 0);
    av_opt_set_int(pl->resampler, "exact_rational", 0, 0);

    return PLAYER_NO_ERROR && !swr_init(pl->resampler);
}

player_err_t player_get_20ms_audio(player_t* pl, uint8_t* out_data, size_t *out_size) {
    // dont read if a packet is held already
    if(!(pl->pkt_held && pl->fmt_eof)) {
        int ret;

        while((ret = av_read_frame(pl->demuxer, pl->pkt)) == 0) {
            // only select packets of the audio stream.
            if(pl->pkt->stream_index == pl->stream->index)
                break;
            else
                continue;
        }

        pl->fmt_eof = ret < 0;  // store state of EOF
    }

    // if it was not sent then hold the packet.
    pl->pkt_held = (avcodec_send_packet(pl->decoder, pl->pkt) == AVERROR(EAGAIN));

    // read the buffered data of swresample if codec supplies no more.
    if(!pl->codec_eof)
        switch(avcodec_receive_frame(pl->decoder, pl->in_frame)) {
            case 0: break;
            case AVERROR(EAGAIN):
                *out_size = 0;
                return PLAYER_NO_ERROR;
            case AVERROR_EOF:
                pl->codec_eof = true;
                break;
            default:
                return PLAYER_MEDIA_DECODE_ERROR;
        }

    if(!pl->codec_eof)
        if(swr_convert_frame(pl->resampler, NULL, pl->in_frame) != 0)
            return PLAYER_MEDIA_INTERNAL_ERROR;
    
    if(swr_convert_frame(pl->resampler, pl->out_frame, NULL) != 0)
        return PLAYER_MEDIA_INTERNAL_ERROR;

    // copy the out_frame data to the passed buffer.
    *out_size = pl->out_frame->nb_samples * 4; // stereo, 16-bit
    memcpy(out_data, pl->out_frame->extended_data[0], *out_size);

    return (*out_size == 0) ? PLAYER_MEDIA_EOF : PLAYER_NO_ERROR;
}

void player_uninit(player_t* pl) {
    swr_free(&pl->resampler);

    av_frame_free(&pl->in_frame);
    av_frame_free(&pl->out_frame);
    av_packet_free(&pl->pkt);
    
    avcodec_free_context(&pl->decoder);
    avformat_free_context(pl->demuxer);
}