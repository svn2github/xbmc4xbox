/*
 * Id Quake II CIN File Demuxer
 * Copyright (c) 2003 The ffmpeg Project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * @file idcin.c
 * Id Quake II CIN file demuxer by Mike Melanson (melanson@pcisys.net)
 * For more information about the Id CIN format, visit:
 *   http://www.csse.monash.edu.au/~timf/
 *
 * CIN is a somewhat quirky and ill-defined format. Here are some notes
 * for anyone trying to understand the technical details of this format:
 *
 * The format has no definite file signature. This is problematic for a
 * general-purpose media player that wants to automatically detect file
 * types. However, a CIN file does start with 5 32-bit numbers that
 * specify audio and video parameters. This demuxer gets around the lack
 * of file signature by performing sanity checks on those parameters.
 * Probabalistically, this is a reasonable solution since the number of
 * valid combinations of the 5 parameters is a very small subset of the
 * total 160-bit number space.
 *
 * Refer to the function idcin_probe() for the precise A/V parameters
 * that this demuxer allows.
 *
 * Next, each audio and video frame has a duration of 1/14 sec. If the
 * audio sample rate is a multiple of the common frequency 22050 Hz it will
 * divide evenly by 14. However, if the sample rate is 11025 Hz:
 *   11025 (samples/sec) / 14 (frames/sec) = 787.5 (samples/frame)
 * The way the CIN stores audio in this case is by storing 787 sample
 * frames in the first audio frame and 788 sample frames in the second
 * audio frame. Therefore, the total number of bytes in an audio frame
 * is given as:
 *   audio frame #0: 787 * (bytes/sample) * (# channels) bytes in frame
 *   audio frame #1: 788 * (bytes/sample) * (# channels) bytes in frame
 *   audio frame #2: 787 * (bytes/sample) * (# channels) bytes in frame
 *   audio frame #3: 788 * (bytes/sample) * (# channels) bytes in frame
 *
 * Finally, not all Id CIN creation tools agree on the resolution of the
 * color palette, apparently. Some creation tools specify red, green, and
 * blue palette components in terms of 6-bit VGA color DAC values which
 * range from 0..63. Other tools specify the RGB components as full 8-bit
 * values that range from 0..255. Since there are no markers in the file to
 * differentiate between the two variants, this demuxer uses the following
 * heuristic:
 *   - load the 768 palette bytes from disk
 *   - assume that they will need to be shifted left by 2 bits to
 *     transform them from 6-bit values to 8-bit values
 *   - scan through all 768 palette bytes
 *     - if any bytes exceed 63, do not shift the bytes at all before
 *       transmitting them to the video decoder
 */

#include "avformat.h"

#define HUFFMAN_TABLE_SIZE (64 * 1024)
#define FRAME_PTS_INC (90000 / 14)

typedef struct IdcinDemuxContext {
    int video_stream_index;
    int audio_stream_index;
    int audio_chunk_size1;
    int audio_chunk_size2;

    /* demux state variables */
    int current_audio_chunk;
    int next_chunk_is_video;
    int audio_present;

    int64_t pts;

    AVPaletteControl palctrl;
} IdcinDemuxContext;

static int idcin_probe(AVProbeData *p)
{
    unsigned int number;

    /*
     * This is what you could call a "probabilistic" file check: Id CIN
     * files don't have a definite file signature. In lieu of such a marker,
     * perform sanity checks on the 5 32-bit header fields:
     *  width, height: greater than 0, less than or equal to 1024
     * audio sample rate: greater than or equal to 8000, less than or
     *  equal to 48000, or 0 for no audio
     * audio sample width (bytes/sample): 0 for no audio, or 1 or 2
     * audio channels: 0 for no audio, or 1 or 2
     */

    /* cannot proceed without 20 bytes */
    if (p->buf_size < 20)
        return 0;

    /* check the video width */
    number = LE_32(&p->buf[0]);
    if ((number == 0) || (number > 1024))
       return 0;

    /* check the video height */
    number = LE_32(&p->buf[4]);
    if ((number == 0) || (number > 1024))
       return 0;

    /* check the audio sample rate */
    number = LE_32(&p->buf[8]);
    if ((number != 0) && ((number < 8000) | (number > 48000)))
        return 0;

    /* check the audio bytes/sample */
    number = LE_32(&p->buf[12]);
    if (number > 2)
        return 0;

    /* check the audio channels */
    number = LE_32(&p->buf[16]);
    if (number > 2)
        return 0;

    /* return half certainly since this check is a bit sketchy */
    return AVPROBE_SCORE_MAX / 2;
}

static int idcin_read_header(AVFormatContext *s,
                             AVFormatParameters *ap)
{
    ByteIOContext *pb = &s->pb;
    IdcinDemuxContext *idcin = (IdcinDemuxContext *)s->priv_data;
    AVStream *st;
    unsigned int width, height;
    unsigned int sample_rate, bytes_per_sample, channels;

    /* get the 5 header parameters */
    width = get_le32(pb);
    height = get_le32(pb);
    sample_rate = get_le32(pb);
    bytes_per_sample = get_le32(pb);
    channels = get_le32(pb);

    st = av_new_stream(s, 0);
    if (!st)
        return AVERROR_NOMEM;
    idcin->video_stream_index = st->index;
    st->codec.codec_type = CODEC_TYPE_VIDEO;
    st->codec.codec_id = CODEC_ID_IDCIN;
    st->codec.codec_tag = 0;  /* no fourcc */
    st->codec.width = width;
    st->codec.height = height;

    /* load up the Huffman tables into extradata */
    st->codec.extradata_size = HUFFMAN_TABLE_SIZE;
    st->codec.extradata = av_malloc(HUFFMAN_TABLE_SIZE);
    if (get_buffer(pb, st->codec.extradata, HUFFMAN_TABLE_SIZE) !=
        HUFFMAN_TABLE_SIZE)
        return -EIO;
    /* save a reference in order to transport the palette */
    st->codec.palctrl = &idcin->palctrl;

    /* if sample rate is 0, assume no audio */
    if (sample_rate) {
        idcin->audio_present = 1;
        st = av_new_stream(s, 0);
        if (!st)
            return AVERROR_NOMEM;
        idcin->audio_stream_index = st->index;
        st->codec.codec_type = CODEC_TYPE_AUDIO;
        st->codec.codec_tag = 1;
        st->codec.channels = channels;
        st->codec.sample_rate = sample_rate;
        st->codec.bits_per_sample = bytes_per_sample * 8;
        st->codec.bit_rate = sample_rate * bytes_per_sample * 8 * channels;
        st->codec.block_align = bytes_per_sample * channels;
        if (bytes_per_sample == 1)
            st->codec.codec_id = CODEC_ID_PCM_U8;
        else
            st->codec.codec_id = CODEC_ID_PCM_S16LE;

        if (sample_rate % 14 != 0) {
            idcin->audio_chunk_size1 = (sample_rate / 14) *
            bytes_per_sample * channels;
            idcin->audio_chunk_size2 = (sample_rate / 14 + 1) *
                bytes_per_sample * channels;
        } else {
            idcin->audio_chunk_size1 = idcin->audio_chunk_size2 =
                (sample_rate / 14) * bytes_per_sample * channels;
        }
        idcin->current_audio_chunk = 0;
    } else
        idcin->audio_present = 1;

    idcin->next_chunk_is_video = 1;
    idcin->pts = 0;

    /* set the pts reference (1 pts = 1/90000) */
    s->pts_num = 1;
    s->pts_den = 90000;

    return 0;
}

static int idcin_read_packet(AVFormatContext *s,
                             AVPacket *pkt)
{
    int ret;
    unsigned int command;
    unsigned int chunk_size;
    IdcinDemuxContext *idcin = (IdcinDemuxContext *)s->priv_data;
    ByteIOContext *pb = &s->pb;
    int i;
    int palette_scale;
    unsigned char r, g, b;
    unsigned char palette_buffer[768];

    if (url_feof(&s->pb))
        return -EIO;

    if (idcin->next_chunk_is_video) {
        command = get_le32(pb);
        if (command == 2) {
            return -EIO;
        } else if (command == 1) {
            /* trigger a palette change */
            idcin->palctrl.palette_changed = 1;
            if (get_buffer(pb, palette_buffer, 768) != 768)
                return -EIO;
            /* scale the palette as necessary */
            palette_scale = 2;
            for (i = 0; i < 768; i++)
                if (palette_buffer[i] > 63) {
                    palette_scale = 0;
                    break;
                }

            for (i = 0; i < 256; i++) {
                r = palette_buffer[i * 3    ] << palette_scale;
                g = palette_buffer[i * 3 + 1] << palette_scale;
                b = palette_buffer[i * 3 + 2] << palette_scale;
                idcin->palctrl.palette[i] = (r << 16) | (g << 8) | (b);
            }
        }

        chunk_size = get_le32(pb);
        /* skip the number of decoded bytes (always equal to width * height) */
        url_fseek(pb, 4, SEEK_CUR);
        chunk_size -= 4;
        if (av_new_packet(pkt, chunk_size))
            ret = -EIO;
        pkt->stream_index = idcin->video_stream_index;
        pkt->pts = idcin->pts;
        ret = get_buffer(pb, pkt->data, chunk_size);
        if (ret != chunk_size)
            ret = -EIO;
    } else {
        /* send out the audio chunk */
        if (idcin->current_audio_chunk)
            chunk_size = idcin->audio_chunk_size2;
        else
            chunk_size = idcin->audio_chunk_size1;
        if (av_new_packet(pkt, chunk_size))
            return -EIO;
        pkt->stream_index = idcin->audio_stream_index;
        pkt->pts = idcin->pts;
        ret = get_buffer(&s->pb, pkt->data, chunk_size);
        if (ret != chunk_size)
            ret = -EIO;

        idcin->current_audio_chunk ^= 1;
        idcin->pts += FRAME_PTS_INC;
    }

    if (idcin->audio_present)
        idcin->next_chunk_is_video ^= 1;

    return ret;
}

static int idcin_read_close(AVFormatContext *s)
{

    return 0;
}

static AVInputFormat idcin_iformat = {
    "idcin",
    "Id CIN format",
    sizeof(IdcinDemuxContext),
    idcin_probe,
    idcin_read_header,
    idcin_read_packet,
    idcin_read_close,
};

int idcin_init(void)
{
    av_register_input_format(&idcin_iformat);
    return 0;
}
