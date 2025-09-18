#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <cstdint>
#include "convertm4atowav.h"
#include "audio_player.h"
#include "vfs.h"

#ifdef __linux__
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

// FFmpeg initialization helper
static bool g_ffmpeg_initialized = false;

static bool initializeFFmpeg() {
    if (g_ffmpeg_initialized) return true;
    
    // Register all formats and codecs (for older FFmpeg versions)
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
    avcodec_register_all();
#endif
    
    g_ffmpeg_initialized = true;
    return true;
}

// Memory-based I/O context for FFmpeg
struct MemoryIOContext {
    const uint8_t* data;
    size_t size;
    size_t pos;
};

static int memory_read(void* opaque, uint8_t* buf, int buf_size) {
    MemoryIOContext* ctx = (MemoryIOContext*)opaque;
    int bytes_to_read = buf_size;
    
    if (ctx->pos >= ctx->size) {
        return AVERROR_EOF;
    }
    
    if (ctx->pos + bytes_to_read > ctx->size) {
        bytes_to_read = ctx->size - ctx->pos;
    }
    
    if (bytes_to_read > 0) {
        memcpy(buf, ctx->data + ctx->pos, bytes_to_read);
        ctx->pos += bytes_to_read;
    }
    
    return bytes_to_read;
}

static int64_t memory_seek(void* opaque, int64_t offset, int whence) {
    MemoryIOContext* ctx = (MemoryIOContext*)opaque;
    
    switch (whence) {
        case SEEK_SET:
            ctx->pos = offset;
            break;
        case SEEK_CUR:
            ctx->pos += offset;
            break;
        case SEEK_END:
            ctx->pos = ctx->size + offset;
            break;
        case AVSEEK_SIZE:
            return ctx->size;
        default:
            return -1;
    }
    
    if (ctx->pos > ctx->size) {
        ctx->pos = ctx->size;
    }
    
    return ctx->pos;
}

bool convertM4aToWavInMemory(const std::vector<uint8_t>& m4a_data, std::vector<uint8_t>& wav_data) {
    if (!initializeFFmpeg()) {
        return false;
    }
    
    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVIOContext* avio_ctx = nullptr;
    SwrContext* swr_ctx = nullptr;
    uint8_t* avio_buffer = nullptr;
    
    // Set up memory I/O context
    MemoryIOContext mem_ctx;
    mem_ctx.data = m4a_data.data();
    mem_ctx.size = m4a_data.size();
    mem_ctx.pos = 0;
    
    const int avio_buffer_size = 4096;
    avio_buffer = (uint8_t*)av_malloc(avio_buffer_size);
    if (!avio_buffer) {
        printf("Failed to allocate AVIO buffer\n");
        return false;
    }
    
    avio_ctx = avio_alloc_context(avio_buffer, avio_buffer_size, 0, &mem_ctx, 
                                  memory_read, nullptr, memory_seek);
    if (!avio_ctx) {
        printf("Failed to create AVIO context\n");
        av_free(avio_buffer);
        return false;
    }
    
    // Allocate format context
    format_ctx = avformat_alloc_context();
    if (!format_ctx) {
        printf("Failed to allocate format context\n");
        avio_context_free(&avio_ctx);
        return false;
    }
    
    format_ctx->pb = avio_ctx;
    
    // Open input
    if (avformat_open_input(&format_ctx, nullptr, nullptr, nullptr) < 0) {
        printf("Failed to open M4A data\n");
        avformat_free_context(format_ctx);
        avio_context_free(&avio_ctx);
        return false;
    }
    
    // Find stream info
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        printf("Failed to find stream info\n");
        avformat_close_input(&format_ctx);
        avio_context_free(&avio_ctx);
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
        avio_context_free(&avio_ctx);
        return false;
    }
    
    AVStream* audio_stream = format_ctx->streams[audio_stream_index];
    AVCodecParameters* codecpar = audio_stream->codecpar;
    
    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        printf("Codec not found\n");
        avformat_close_input(&format_ctx);
        avio_context_free(&avio_ctx);
        return false;
    }
    
    // Create codec context
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        printf("Failed to allocate codec context\n");
        avformat_close_input(&format_ctx);
        avio_context_free(&avio_ctx);
        return false;
    }
    
    // Copy codec parameters
    if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
        printf("Failed to copy codec parameters\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        avio_context_free(&avio_ctx);
        return false;
    }
    
    // Open codec
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        printf("Failed to open codec\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        avio_context_free(&avio_ctx);
        return false;
    }
    
    // Get channel count (compatibility with different FFmpeg versions)
    int channels;
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
    channels = codec_ctx->ch_layout.nb_channels;
#else
    channels = codec_ctx->channels;
#endif
    
    printf("M4A (memory): %d Hz, %d channels, %d bits per sample\n", 
           codec_ctx->sample_rate, channels, 
           av_get_bytes_per_sample(codec_ctx->sample_fmt) * 8);
    
    // Set up resampler for conversion to 16-bit PCM
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        printf("Failed to allocate resampler\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        avio_context_free(&avio_ctx);
        return false;
    }
    
    // Configure resampler
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
    AVChannelLayout in_ch_layout, out_ch_layout;
    av_channel_layout_copy(&in_ch_layout, &codec_ctx->ch_layout);
    av_channel_layout_default(&out_ch_layout, channels);
    
    av_opt_set_chlayout(swr_ctx, "in_chlayout", &in_ch_layout, 0);
    av_opt_set_chlayout(swr_ctx, "out_chlayout", &out_ch_layout, 0);
#else
    av_opt_set_int(swr_ctx, "in_channel_layout", codec_ctx->channel_layout ? codec_ctx->channel_layout : av_get_default_channel_layout(channels), 0);
    av_opt_set_int(swr_ctx, "out_channel_layout", av_get_default_channel_layout(channels), 0);
#endif
    
    av_opt_set_int(swr_ctx, "in_sample_rate", codec_ctx->sample_rate, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", codec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", codec_ctx->sample_fmt, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    
    if (swr_init(swr_ctx) < 0) {
        printf("Failed to initialize resampler\n");
        swr_free(&swr_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        avio_context_free(&avio_ctx);
        return false;
    }
    
    // Prepare WAV data vector
    wav_data.clear();
    wav_data.reserve(m4a_data.size() * 2); // Rough estimate
    
    // Helper function to append data to vector
    auto append_bytes = [&wav_data](const void* data, size_t size) {
        const uint8_t* bytes = (const uint8_t*)data;
        wav_data.insert(wav_data.end(), bytes, bytes + size);
    };
    
    // Write WAV header (we'll update the sizes later)
    size_t riff_size_pos, data_size_pos;
    
    append_bytes("RIFF", 4);
    riff_size_pos = wav_data.size();
    uint32_t placeholder = 0;
    append_bytes(&placeholder, 4); // File size (will update later)
    append_bytes("WAVE", 4);
    
    // fmt chunk
    append_bytes("fmt ", 4);
    int fmt_chunk_size = 16;
    short audio_format = 1; // PCM
    short wav_channels = (short)channels;
    int sample_rate = codec_ctx->sample_rate;
    short bits_per_sample = 16;
    int byte_rate = sample_rate * channels * bits_per_sample / 8;
    short block_align = channels * bits_per_sample / 8;
    
    append_bytes(&fmt_chunk_size, 4);
    append_bytes(&audio_format, 2);
    append_bytes(&wav_channels, 2);
    append_bytes(&sample_rate, 4);
    append_bytes(&byte_rate, 4);
    append_bytes(&block_align, 2);
    append_bytes(&bits_per_sample, 2);
    
    // data chunk header
    append_bytes("data", 4);
    data_size_pos = wav_data.size();
    append_bytes(&placeholder, 4); // Data size (will update later)
    
    size_t audio_data_start = wav_data.size();
    
    // Decode and convert audio data
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    
    if (!packet || !frame) {
        printf("Failed to allocate packet or frame\n");
        if (packet) av_packet_free(&packet);
        if (frame) av_frame_free(&frame);
        swr_free(&swr_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        avio_context_free(&avio_ctx);
        return false;
    }
    
    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == audio_stream_index) {
            if (avcodec_send_packet(codec_ctx, packet) >= 0) {
                while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
                    // Convert to 16-bit PCM
                    uint8_t* output_buffer = nullptr;
                    int output_samples = swr_convert(swr_ctx, &output_buffer, frame->nb_samples,
                                                   (const uint8_t**)frame->data, frame->nb_samples);
                    
                    if (output_samples > 0) {
                        int output_size = output_samples * channels * sizeof(int16_t);
                        if (!output_buffer) {
                            // Allocate buffer if swr_convert didn't do it
                            output_buffer = (uint8_t*)av_malloc(output_size);
                            if (output_buffer) {
                                swr_convert(swr_ctx, &output_buffer, frame->nb_samples,
                                          (const uint8_t**)frame->data, frame->nb_samples);
                            }
                        }
                        
                        if (output_buffer) {
                            append_bytes(output_buffer, output_size);
                            av_freep(&output_buffer);
                        }
                    }
                }
            }
        }
        av_packet_unref(packet);
    }
    
    // Flush decoder
    avcodec_send_packet(codec_ctx, nullptr);
    while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
        uint8_t* output_buffer = nullptr;
        int output_samples = swr_convert(swr_ctx, &output_buffer, frame->nb_samples,
                                       (const uint8_t**)frame->data, frame->nb_samples);
        
        if (output_samples > 0 && output_buffer) {
            int output_size = output_samples * channels * sizeof(int16_t);
            append_bytes(output_buffer, output_size);
            av_freep(&output_buffer);
        }
    }
    
    // Update WAV header with actual sizes
    uint32_t data_size = (uint32_t)(wav_data.size() - audio_data_start);
    uint32_t file_size = (uint32_t)(wav_data.size() - 8);
    
    // Update RIFF chunk size
    memcpy(&wav_data[riff_size_pos], &file_size, 4);
    
    // Update data chunk size
    memcpy(&wav_data[data_size_pos], &data_size, 4);
    
    // Cleanup
    av_packet_free(&packet);
    av_frame_free(&frame);
    swr_free(&swr_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    avio_context_free(&avio_ctx);
    
    printf("M4A to WAV memory conversion complete (%zu bytes)\n", wav_data.size());
    return true;
}

bool convertM4aToWav(const char* m4a_filename, const char* wav_filename) {
    if (!initializeFFmpeg()) {
        return false;
    }
    
    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    SwrContext* swr_ctx = nullptr;
    
    // Open input file
    if (avformat_open_input(&format_ctx, m4a_filename, nullptr, nullptr) < 0) {
        printf("Cannot open M4A file: %s\n", m4a_filename);
        return false;
    }
    
    // Find stream info
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        printf("Failed to find stream info\n");
        avformat_close_input(&format_ctx);
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
        return false;
    }
    
    AVStream* audio_stream = format_ctx->streams[audio_stream_index];
    AVCodecParameters* codecpar = audio_stream->codecpar;
    
    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        printf("Codec not found\n");
        avformat_close_input(&format_ctx);
        return false;
    }
    
    // Create codec context
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        printf("Failed to allocate codec context\n");
        avformat_close_input(&format_ctx);
        return false;
    }
    
    // Copy codec parameters
    if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
        printf("Failed to copy codec parameters\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }
    
    // Open codec
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        printf("Failed to open codec\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }
    
    // Get channel count (compatibility with different FFmpeg versions)
    int channels;
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
    channels = codec_ctx->ch_layout.nb_channels;
#else
    channels = codec_ctx->channels;
#endif
    
    printf("M4A: %d Hz, %d channels, %d bits per sample\n", 
           codec_ctx->sample_rate, channels, 
           av_get_bytes_per_sample(codec_ctx->sample_fmt) * 8);
    
    // Set up resampler
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        printf("Failed to allocate resampler\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }
    
    // Configure resampler (same as memory version)
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
    AVChannelLayout in_ch_layout, out_ch_layout;
    av_channel_layout_copy(&in_ch_layout, &codec_ctx->ch_layout);
    av_channel_layout_default(&out_ch_layout, channels);
    
    av_opt_set_chlayout(swr_ctx, "in_chlayout", &in_ch_layout, 0);
    av_opt_set_chlayout(swr_ctx, "out_chlayout", &out_ch_layout, 0);
#else
    av_opt_set_int(swr_ctx, "in_channel_layout", codec_ctx->channel_layout ? codec_ctx->channel_layout : av_get_default_channel_layout(channels), 0);
    av_opt_set_int(swr_ctx, "out_channel_layout", av_get_default_channel_layout(channels), 0);
#endif
    
    av_opt_set_int(swr_ctx, "in_sample_rate", codec_ctx->sample_rate, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", codec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", codec_ctx->sample_fmt, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    
    if (swr_init(swr_ctx) < 0) {
        printf("Failed to initialize resampler\n");
        swr_free(&swr_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }
    
    // Create WAV file
    FILE* wav_file = fopen(wav_filename, "wb");
    if (!wav_file) {
        printf("Cannot create WAV file: %s\n", wav_filename);
        swr_free(&swr_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }
    
    // Write WAV header (we'll update sizes later)
    long riff_size_pos, data_size_pos;
    
    // RIFF header
    fwrite("RIFF", 1, 4, wav_file);
    riff_size_pos = ftell(wav_file);
    uint32_t placeholder = 0;
    fwrite(&placeholder, 4, 1, wav_file); // File size (will update later)
    fwrite("WAVE", 1, 4, wav_file);
    
    // fmt chunk
    fwrite("fmt ", 1, 4, wav_file);
    int fmt_chunk_size = 16;
    short audio_format = 1; // PCM
    short wav_channels = (short)channels;
    int sample_rate = codec_ctx->sample_rate;
    short bits_per_sample = 16;
    int byte_rate = sample_rate * channels * bits_per_sample / 8;
    short block_align = channels * bits_per_sample / 8;
    
    fwrite(&fmt_chunk_size, 4, 1, wav_file);
    fwrite(&audio_format, 2, 1, wav_file);
    fwrite(&wav_channels, 2, 1, wav_file);
    fwrite(&sample_rate, 4, 1, wav_file);
    fwrite(&byte_rate, 4, 1, wav_file);
    fwrite(&block_align, 2, 1, wav_file);
    fwrite(&bits_per_sample, 2, 1, wav_file);
    
    // data chunk header
    fwrite("data", 1, 4, wav_file);
    data_size_pos = ftell(wav_file);
    fwrite(&placeholder, 4, 1, wav_file); // Data size (will update later)
    
    long audio_data_start = ftell(wav_file);
    uint32_t total_bytes_written = 0;
    
    // Decode and convert audio data
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    
    if (!packet || !frame) {
        printf("Failed to allocate packet or frame\n");
        if (packet) av_packet_free(&packet);
        if (frame) av_frame_free(&frame);
        fclose(wav_file);
        swr_free(&swr_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }
    
    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == audio_stream_index) {
            if (avcodec_send_packet(codec_ctx, packet) >= 0) {
                while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
                    uint8_t* output_buffer = nullptr;
                    int output_samples = swr_convert(swr_ctx, &output_buffer, frame->nb_samples,
                                                   (const uint8_t**)frame->data, frame->nb_samples);
                    
                    if (output_samples > 0) {
                        int output_size = output_samples * channels * sizeof(int16_t);
                        if (!output_buffer) {
                            output_buffer = (uint8_t*)av_malloc(output_size);
                            if (output_buffer) {
                                swr_convert(swr_ctx, &output_buffer, frame->nb_samples,
                                          (const uint8_t**)frame->data, frame->nb_samples);
                            }
                        }
                        
                        if (output_buffer) {
                            fwrite(output_buffer, 1, output_size, wav_file);
                            total_bytes_written += output_size;
                            av_freep(&output_buffer);
                        }
                    }
                }
            }
        }
        av_packet_unref(packet);
    }
    
    // Flush decoder
    avcodec_send_packet(codec_ctx, nullptr);
    while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
        uint8_t* output_buffer = nullptr;
        int output_samples = swr_convert(swr_ctx, &output_buffer, frame->nb_samples,
                                       (const uint8_t**)frame->data, frame->nb_samples);
        
        if (output_samples > 0 && output_buffer) {
            int output_size = output_samples * channels * sizeof(int16_t);
            fwrite(output_buffer, 1, output_size, wav_file);
            total_bytes_written += output_size;
            av_freep(&output_buffer);
        }
    }
    
    // Update WAV header with actual sizes
    uint32_t file_size = total_bytes_written + 36; // Audio data + header size - 8
    
    // Update RIFF chunk size
    fseek(wav_file, riff_size_pos, SEEK_SET);
    fwrite(&file_size, 4, 1, wav_file);
    
    // Update data chunk size
    fseek(wav_file, data_size_pos, SEEK_SET);
    fwrite(&total_bytes_written, 4, 1, wav_file);
    
    fclose(wav_file);
    
    // Cleanup
    av_packet_free(&packet);
    av_frame_free(&frame);
    swr_free(&swr_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    
    printf("M4A conversion complete\n");
    return true;
}

bool convert_m4a_to_wav(AudioPlayer *player, const char* filename) {
    // Check cache first
    const char* cached_file = get_cached_conversion(&player->conversion_cache, filename);
    if (cached_file) {
        strncpy(player->temp_wav_file, cached_file, sizeof(player->temp_wav_file) - 1);
        player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
        return true;
    }
    
    // Generate a unique virtual filename
    static int virtual_counter = 0;
    char virtual_filename[256];
    snprintf(virtual_filename, sizeof(virtual_filename), "virtual_m4a_%d.wav", virtual_counter++);
    
    strncpy(player->temp_wav_file, virtual_filename, sizeof(player->temp_wav_file) - 1);
    player->temp_wav_file[sizeof(player->temp_wav_file) - 1] = '\0';
    
    printf("Converting M4A to virtual WAV: %s -> %s\n", filename, virtual_filename);
    
    // Read M4A file into memory
    FILE* m4a_file = fopen(filename, "rb");
    if (!m4a_file) {
        printf("Cannot open M4A file: %s\n", filename);
        return false;
    }
    
    fseek(m4a_file, 0, SEEK_END);
    long m4a_size = ftell(m4a_file);
    fseek(m4a_file, 0, SEEK_SET);
    
    std::vector<uint8_t> m4a_data(m4a_size);
    if (fread(m4a_data.data(), 1, m4a_size, m4a_file) != (size_t)m4a_size) {
        printf("Failed to read M4A file\n");
        fclose(m4a_file);
        return false;
    }
    fclose(m4a_file);
    
    // Convert M4A to WAV in memory
    std::vector<uint8_t> wav_data;
    if (!convertM4aToWavInMemory(m4a_data, wav_data)) {
        printf("M4A to WAV conversion failed\n");
        return false;
    }
    
    // Create virtual file and write WAV data
    VirtualFile* vf = create_virtual_file(virtual_filename);
    if (!vf) {
        printf("Cannot create virtual WAV file: %s\n", virtual_filename);
        return false;
    }
    
    if (!virtual_file_write(vf, wav_data.data(), wav_data.size())) {
        printf("Failed to write virtual WAV file\n");
        return false;
    }
    
    // Add to cache after successful conversion
    add_to_conversion_cache(&player->conversion_cache, filename, virtual_filename);
    
    printf("M4A conversion to virtual file complete\n");
    return true;
}

#else // !__linux__

// Stub implementations for non-Linux platforms
bool convertM4aToWavInMemory(const std::vector<uint8_t>& m4a_data, std::vector<uint8_t>& wav_data) {
    printf("M4A conversion not supported on this platform (Linux FFmpeg implementation)\n");
    return false;
}

bool convertM4aToWav(const char* m4a_filename, const char* wav_filename) {
    printf("M4A conversion not supported on this platform (Linux FFmpeg implementation)\n");
    return false;
}

bool convert_m4a_to_wav(AudioPlayer *player, const char* filename) {
    printf("M4A conversion not supported on this platform (Linux FFmpeg implementation)\n");
    return false;
}

#endif // __linux__
