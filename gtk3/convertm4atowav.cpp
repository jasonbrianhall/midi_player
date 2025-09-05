#include "convertm4atowav.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
}

// WAV header structure
struct WAVHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunk_size;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmt_size = 16;
    uint16_t audio_format = 1;  // PCM
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t data_size;
};

// Custom IO context for reading from memory buffer
struct BufferData {
    const uint8_t *ptr;
    size_t size;
    size_t pos;
};

static int read_packet(void *opaque, uint8_t *buf, int buf_size) {
    BufferData *bd = (BufferData *)opaque;
    buf_size = std::min(buf_size, (int)(bd->size - bd->pos));
    
    if (buf_size <= 0) {
        return AVERROR_EOF;
    }
    
    memcpy(buf, bd->ptr + bd->pos, buf_size);
    bd->pos += buf_size;
    return buf_size;
}

static int64_t seek_packet(void *opaque, int64_t offset, int whence) {
    BufferData *bd = (BufferData *)opaque;
    
    switch (whence) {
        case SEEK_SET:
            bd->pos = offset;
            break;
        case SEEK_CUR:
            bd->pos += offset;
            break;
        case SEEK_END:
            bd->pos = bd->size + offset;
            break;
        case AVSEEK_SIZE:
            return bd->size;
        default:
            return -1;
    }
    
    if (bd->pos > bd->size) {
        bd->pos = bd->size;
    }
    
    return bd->pos;
}

bool convertM4aToWavInMemory(const std::vector<uint8_t>& m4a_data, std::vector<uint8_t>& wav_data) {
    AVFormatContext *format_ctx = nullptr;
    AVCodecContext *codec_ctx = nullptr;
    SwrContext *swr_ctx = nullptr;
    AVIOContext *avio_ctx = nullptr;
    uint8_t *avio_buffer = nullptr;
    
    // Initialize FFmpeg
    av_register_all();
    avcodec_register_all();
    
    try {
        // Set up custom IO context for reading from memory
        BufferData buffer_data;
        buffer_data.ptr = m4a_data.data();
        buffer_data.size = m4a_data.size();
        buffer_data.pos = 0;
        
        const int avio_buffer_size = 4096;
        avio_buffer = (uint8_t*)av_malloc(avio_buffer_size);
        if (!avio_buffer) {
            printf("Failed to allocate AVIO buffer\n");
            return false;
        }
        
        avio_ctx = avio_alloc_context(avio_buffer, avio_buffer_size, 0, &buffer_data, read_packet, nullptr, seek_packet);
        if (!avio_ctx) {
            printf("Failed to create AVIO context\n");
            av_free(avio_buffer);
            return false;
        }
        
        // Allocate format context
        format_ctx = avformat_alloc_context();
        if (!format_ctx) {
            printf("Failed to allocate format context\n");
            av_free(avio_ctx);
            av_free(avio_buffer);
            return false;
        }
        
        format_ctx->pb = avio_ctx;
        
        // Open input
        if (avformat_open_input(&format_ctx, nullptr, nullptr, nullptr) < 0) {
            printf("Failed to open M4A input\n");
            avformat_free_context(format_ctx);
            av_free(avio_ctx);
            av_free(avio_buffer);
            return false;
        }
        
        // Find stream info
        if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
            printf("Failed to find stream info\n");
            avformat_close_input(&format_ctx);
            av_free(avio_ctx);
            av_free(avio_buffer);
            return false;
        }
        
        // Find audio stream
        int audio_stream_index = -1;
        for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
            if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                audio_stream_index = i;
                break;
            }
        }
        
        if (audio_stream_index == -1) {
            printf("No audio stream found\n");
            avformat_close_input(&format_ctx);
            av_free(avio_ctx);
            av_free(avio_buffer);
            return false;
        }
        
        AVStream *audio_stream = format_ctx->streams[audio_stream_index];
        AVCodecParameters *codecpar = audio_stream->codecpar;
        
        // Find decoder
        AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
        if (!codec) {
            printf("Codec not found\n");
            avformat_close_input(&format_ctx);
            av_free(avio_ctx);
            av_free(avio_buffer);
            return false;
        }
        
        // Allocate codec context
        codec_ctx = avcodec_alloc_context3(codec);
        if (!codec_ctx) {
            printf("Failed to allocate codec context\n");
            avformat_close_input(&format_ctx);
            av_free(avio_ctx);
            av_free(avio_buffer);
            return false;
        }
        
        // Copy codec parameters
        if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
            printf("Failed to copy codec parameters\n");
            avcodec_free_context(&codec_ctx);
            avformat_close_input(&format_ctx);
            av_free(avio_ctx);
            av_free(avio_buffer);
            return false;
        }
        
        // Open codec
        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
            printf("Failed to open codec\n");
            avcodec_free_context(&codec_ctx);
            avformat_close_input(&format_ctx);
            av_free(avio_ctx);
            av_free(avio_buffer);
            return false;
        }
        
        // Set up resampler for conversion to 16-bit stereo PCM
        swr_ctx = swr_alloc();
        if (!swr_ctx) {
            printf("Failed to allocate resampler\n");
            avcodec_free_context(&codec_ctx);
            avformat_close_input(&format_ctx);
            av_free(avio_ctx);
            av_free(avio_buffer);
            return false;
        }
        
        // Configure resampler
        av_opt_set_int(swr_ctx, "in_channel_layout", codec_ctx->channel_layout ? codec_ctx->channel_layout : av_get_default_channel_layout(codec_ctx->channels), 0);
        av_opt_set_int(swr_ctx, "in_sample_rate", codec_ctx->sample_rate, 0);
        av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", codec_ctx->sample_fmt, 0);
        
        av_opt_set_int(swr_ctx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
        av_opt_set_int(swr_ctx, "out_sample_rate", 44100, 0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
        
        if (swr_init(swr_ctx) < 0) {
            printf("Failed to initialize resampler\n");
            swr_free(&swr_ctx);
            avcodec_free_context(&codec_ctx);
            avformat_close_input(&format_ctx);
            av_free(avio_ctx);
            av_free(avio_buffer);
            return false;
        }
        
        // Prepare output buffer
        std::vector<int16_t> pcm_data;
        
        AVPacket packet;
        av_init_packet(&packet);
        
        AVFrame *frame = av_frame_alloc();
        if (!frame) {
            printf("Failed to allocate frame\n");
            swr_free(&swr_ctx);
            avcodec_free_context(&codec_ctx);
            avformat_close_input(&format_ctx);
            av_free(avio_ctx);
            av_free(avio_buffer);
            return false;
        }
        
        // Decode audio
        while (av_read_frame(format_ctx, &packet) >= 0) {
            if (packet.stream_index == audio_stream_index) {
                int ret = avcodec_send_packet(codec_ctx, &packet);
                if (ret < 0) {
                    printf("Error sending packet to decoder\n");
                    break;
                }
                
                while (ret >= 0) {
                    ret = avcodec_receive_frame(codec_ctx, frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        break;
                    } else if (ret < 0) {
                        printf("Error receiving frame from decoder\n");
                        break;
                    }
                    
                    // Calculate output sample count
                    int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
                    if (out_samples <= 0) continue;
                    
                    // Allocate output buffer
                    uint8_t *output_buffer;
                    int output_linesize;
                    ret = av_samples_alloc(&output_buffer, &output_linesize, 2, out_samples, AV_SAMPLE_FMT_S16, 0);
                    if (ret < 0) {
                        printf("Failed to allocate output buffer\n");
                        continue;
                    }
                    
                    // Convert audio
                    int converted_samples = swr_convert(swr_ctx, &output_buffer, out_samples, 
                                                       (const uint8_t**)frame->data, frame->nb_samples);
                    
                    if (converted_samples > 0) {
                        int16_t *samples = (int16_t*)output_buffer;
                        pcm_data.insert(pcm_data.end(), samples, samples + (converted_samples * 2));
                    }
                    
                    av_freep(&output_buffer);
                }
            }
            av_packet_unref(&packet);
        }
        
        // Flush decoder
        avcodec_send_packet(codec_ctx, nullptr);
        int ret;
        while ((ret = avcodec_receive_frame(codec_ctx, frame)) >= 0) {
            int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
            if (out_samples <= 0) continue;
            
            uint8_t *output_buffer;
            int output_linesize;
            ret = av_samples_alloc(&output_buffer, &output_linesize, 2, out_samples, AV_SAMPLE_FMT_S16, 0);
            if (ret < 0) continue;
            
            int converted_samples = swr_convert(swr_ctx, &output_buffer, out_samples, 
                                               (const uint8_t**)frame->data, frame->nb_samples);
            
            if (converted_samples > 0) {
                int16_t *samples = (int16_t*)output_buffer;
                pcm_data.insert(pcm_data.end(), samples, samples + (converted_samples * 2));
            }
            
            av_freep(&output_buffer);
        }
        
        // Create WAV header
        WAVHeader header;
        header.num_channels = 2;
        header.sample_rate = 44100;
        header.byte_rate = 44100 * 2 * 2;  // sample_rate * channels * bytes_per_sample
        header.block_align = 2 * 2;        // channels * bytes_per_sample
        header.data_size = pcm_data.size() * sizeof(int16_t);
        header.chunk_size = 36 + header.data_size;
        
        // Build WAV data
        wav_data.clear();
        wav_data.resize(sizeof(WAVHeader) + header.data_size);
        
        memcpy(wav_data.data(), &header, sizeof(WAVHeader));
        memcpy(wav_data.data() + sizeof(WAVHeader), pcm_data.data(), header.data_size);
        
        // Cleanup
        av_frame_free(&frame);
        swr_free(&swr_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        av_free(avio_ctx);
        av_free(avio_buffer);
        
        printf("M4A conversion successful: %zu PCM samples -> %zu bytes WAV\n", 
               pcm_data.size(), wav_data.size());
        
        return true;
        
    } catch (const std::exception& e) {
        printf("Exception during M4A conversion: %s\n", e.what());
        
        // Cleanup on exception
        if (swr_ctx) swr_free(&swr_ctx);
        if (codec_ctx) avcodec_free_context(&codec_ctx);
        if (format_ctx) avformat_close_input(&format_ctx);
        if (avio_ctx) av_free(avio_ctx);
        if (avio_buffer) av_free(avio_buffer);
        
        return false;
    }
}

bool convertM4aToWav(const char* input_filename, const char* output_filename) {
    // Read input file
    FILE* input_file = fopen(input_filename, "rb");
    if (!input_file) {
        printf("Cannot open input file: %s\n", input_filename);
        return false;
    }
    
    fseek(input_file, 0, SEEK_END);
    long file_size = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);
    
    std::vector<uint8_t> m4a_data(file_size);
    if (fread(m4a_data.data(), 1, file_size, input_file) != (size_t)file_size) {
        printf("Failed to read input file\n");
        fclose(input_file);
        return false;
    }
    fclose(input_file);
    
    // Convert to WAV
    std::vector<uint8_t> wav_data;
    if (!convertM4aToWavInMemory(m4a_data, wav_data)) {
        return false;
    }
    
    // Write output file
    FILE* output_file = fopen(output_filename, "wb");
    if (!output_file) {
        printf("Cannot create output file: %s\n", output_filename);
        return false;
    }
    
    if (fwrite(wav_data.data(), 1, wav_data.size(), output_file) != wav_data.size()) {
        printf("Failed to write output file\n");
        fclose(output_file);
        return false;
    }
    
    fclose(output_file);
    return true;
}
