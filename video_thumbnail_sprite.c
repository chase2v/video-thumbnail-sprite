#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include "save_bmp.h"

int timebase;
int avgFrameRate;
int duration;
int count = 0;
int uw;
int uh;

typedef struct
{
    uint8_t *ptr;
    size_t size;
    size_t totalSize;
    size_t pos;
} buffer_data;

int arrangeThumbnail(uint8_t *images[], SpriteImage *spriteImage, int rows)
{
    printf("1111");

    int imagesLen = count;
    int cols = imagesLen / rows + 1;
    rows = imagesLen < rows ? imagesLen : rows;

    // 先分组
    uint8_t *imageGroups[cols][rows];
    int r = 0;
    int c = 0;
    for (int i = 0; i < imagesLen; i++)
    {
        imageGroups[i / rows][i % rows] = images[i];
    }

    spriteImage->data = (uint8_t *)av_malloc(cols * rows * uw * uh * 3 * sizeof(uint8_t));

    int len = 0;
    int cur = 0;
    for (int i = 0; i < cols; i++)
    {
        for (int j = 0; j < uh; j++)
        {
            for (int k = 0; k < rows; k++)
            {
                for (int l = 0; l < uw * 3; l++)
                {
                    if (i * rows + k < imagesLen)
                    {
                        *(spriteImage->data + len) = *(imageGroups[i][k] + j * uw * 3 + l);
                    }
                    else
                    {
                        // 填充白色
                        *(spriteImage->data + len) = 255;
                    }
                    len++;
                }
            }
        }
    }

    spriteImage->size = len;
    spriteImage->width = uw;
    spriteImage->height = uh;
    spriteImage->rows = cols;
    spriteImage->count = imagesLen;

    return 0;
}

int convertPixFmt(struct SwsContext *img_convert_ctx, AVFrame *frame, AVFrame *pFrameRGB)
{
    printf("1111");

    int w = frame->width;
    int h = frame->height;


    int numBytes=avpicture_get_size(AV_PIX_FMT_RGB24, w, h);
    uint8_t *buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));


    avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24, w, h);

    sws_scale(img_convert_ctx, frame->data, frame->linesize,
              0, h, pFrameRGB->data, pFrameRGB->linesize);

	return 0;
}

int decodePacket(uint8_t *images[], AVCodecContext *avctx, struct SwsContext *swsCtx,
                 AVFrame *frame, int *frameCount, AVPacket *pkt, int interval, int last)
{
    printf("1111");

    int len, gotFrame;

    len = avcodec_decode_video2(avctx, frame, &gotFrame, pkt);
    if (len < 0)
    {
        fprintf(stderr, "Error while decoding frame %d\n", *frameCount);
        return len;
    }
    if (gotFrame && *frameCount % (avgFrameRate * interval) == 0)
    {
        printf("Saving %sframe %3d, %d\n", last ? "last " : "", *frameCount, count);
        fflush(stdout);

        AVFrame *frameRGB;
        frameRGB = av_frame_alloc();
        convertPixFmt(swsCtx, frame, frameRGB);
        uw = frame->width;
        uh = frame->height;
        images[count] = frameRGB->data[0];
        av_frame_free(&frameRGB);
        count++;
    }

    (*frameCount)++;

    if (pkt->data)
    {
        pkt->size -= len;
        pkt->data += len;
    }

    return 0;
}

int initDecoder(AVStream *stream, AVCodecContext *codecCtx, AVCodec *codec,
                struct SwsContext *swsCtx)
{
    /* find decoder for the stream */
    codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec)
    {
        fprintf(stderr, "Failed to find %s codec\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        return AVERROR(EINVAL);
    }

    // codecCtx = avcodec_alloc_context3(NULL);
    // if (!codecCtx)
    // {
    //     fprintf(stderr, "Could not allocate video codec context\n");
    //     exit(1);
    // }

    // /* Copy codec parameters from input stream to output codec context */
    // if (avcodec_parameters_to_context(codecCtx, stream->codecpar) < 0)
    // {
    //     fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
    //             av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
    //     return -1;
    // }

    // /* open it */
    // if (avcodec_open2(codecCtx, codec, NULL) < 0)
    // {
    //     fprintf(stderr, "Could not open codec\n");
    //     exit(1);
    // }

    // swsCtx = sws_getContext(
    //     codecCtx->width,
    //     codecCtx->height,
    //     codecCtx->pix_fmt || AV_PIX_FMT_YUV420P,
    //     codecCtx->width,
    //     codecCtx->height,
    //     AV_PIX_FMT_RGB24,
    //     SWS_BICUBIC, NULL, NULL, NULL);
    // if (swsCtx == NULL)
    // {
    //     fprintf(stderr, "Cannot initialize the conversion context\n");
    //     exit(1);
    // }

    return 0;
}

int initInput(char *inputFilename, AVFormatContext *fmtCtx, int *streamIndex,
              AVFrame *frame, AVPacket *packet)
{
    /* open input */
    if (avformat_open_input(&fmtCtx, "./example.mp4", NULL, NULL) < 0)
    {
        fprintf(stderr, "Could not open input\n");
        exit(1);
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(fmtCtx, NULL) < 0)
    {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    *streamIndex = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (*streamIndex < 0)
    {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        exit(1);
    }

    /* dump input information to stderr */
    av_dump_format(fmtCtx, 0, inputFilename, 0);

    frame = av_frame_alloc();
    if (!frame)
    {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    av_init_packet(packet);

    return 0;
}

int main(int argc, char *argv[])
{
    SpriteImage *spriteImg;
    struct SwsContext *swsCtx;
    AVFormatContext *fmtCtx = NULL;
    AVStream *stream = NULL;
    AVCodecContext *codecCtx;
    AVCodec *codec = NULL;
    AVFrame *frame;
    AVPacket packet;
    int streamIndex;
    int frameCount = 0;
    char *inputFilename, *outputFilename;
    int interval;
    int cols;

    if (argc < 5)
    {
        fprintf(stderr, "Must provide interval and cols");
        exit(1);
    }
    inputFilename = argv[1];
    outputFilename = argv[2];
    interval = argv[3];
    cols = argv[4];

    /* init input */
    if (initInput(inputFilename, fmtCtx, &streamIndex, frame, &packet) < 0)
    {
        fprintf(stderr, "Failed to init input");
        exit(1);
    }

    printf("stream index is: %d\n", streamIndex);
    stream = fmtCtx->streams[streamIndex];
    timebase = stream->time_base.den;
    avgFrameRate = stream->avg_frame_rate.num;
    duration = stream->duration;
    uint8_t *images[duration / (timebase * interval)];

    /* init decoder */
    if (initDecoder(stream, codecCtx, codec, swsCtx) < 0)
    {
        fprintf(stderr, "Failed to init decoder");
        exit(1);
    }

    /* decode packet */
    // int rt = av_read_frame(fmtCtx, &packet);
    // if (rt < 0) {
    //     printf("Could not find data: %s\n", av_err2str(rt));
    //     exit(1);
    // }
    while (av_read_frame(fmtCtx, &packet) >= 0)
    {
        if (packet.stream_index == streamIndex)
        {
            if (decodePacket(images, codecCtx, swsCtx, frame, &frameCount,
                             &packet, interval, 0) < 0)
            {
                exit(1);
            }
        }

        av_packet_unref(&packet);
    }
    packet.data = NULL;
    packet.size = 0;
    decodePacket(images, codecCtx, swsCtx, frame, &frameCount, &packet, interval, 1);

    // save image
    saveBMP(spriteImg, outputFilename);

end:
    free(spriteImg->data);
    free(spriteImg);
    sws_freeContext(swsCtx);
    avcodec_free_context(&codecCtx);
    av_frame_free(&frame);
    avformat_close_input(&fmtCtx);

    return 0;
}