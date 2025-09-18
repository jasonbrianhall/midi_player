#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <cstdint>
#include "audio_player.h"
#include "vfs.h"

#ifdef _WIN32
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

// Media Foundation initialization helper
static bool g_mf_initialized = false;

static bool initializeMediaFoundation() {
    if (g_mf_initialized) return true;
    
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        printf("Failed to initialize COM\n");
        return false;
    }
    
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        printf("Failed to initialize Media Foundation\n");
        CoUninitialize();
        return false;
    }
    
    g_mf_initialized = true;
    return true;
}

static void cleanupMediaFoundation() {
    if (g_mf_initialized) {
        MFShutdown();
        CoUninitialize();
        g_mf_initialized = false;
    }
}

// Utility function to convert string to wstring
static std::wstring stringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &result[0], size);
    return result;
}

bool convertM4aToWavInMemory(const std::vector<uint8_t>& m4a_data, std::vector<uint8_t>& wav_data) {
    // Media Foundation requires actual file access for M4A files
    // Create a temporary file for the conversion process
    char temp_path[MAX_PATH];
    char temp_m4a_file[MAX_PATH];
    
    if (GetTempPathA(MAX_PATH, temp_path) == 0 ||
        GetTempFileNameA(temp_path, "m4a", 0, temp_m4a_file) == 0) {
        printf("Failed to create temporary file path\n");
        return false;
    }
    
    // Write M4A data to temporary file
    FILE* temp_file = fopen(temp_m4a_file, "wb");
    if (!temp_file) {
        printf("Failed to create temporary M4A file\n");
        return false;
    }
    
    if (fwrite(m4a_data.data(), 1, m4a_data.size(), temp_file) != m4a_data.size()) {
        printf("Failed to write M4A data to temporary file\n");
        fclose(temp_file);
        DeleteFileA(temp_m4a_file);
        return false;
    }
    fclose(temp_file);
    
    // Create temporary WAV file
    char temp_wav_file[MAX_PATH];
    if (GetTempFileNameA(temp_path, "wav", 0, temp_wav_file) == 0) {
        printf("Failed to create temporary WAV file path\n");
        DeleteFileA(temp_m4a_file);
        return false;
    }
    
    // Convert using file-based method
    bool success = convertM4aToWav(temp_m4a_file, temp_wav_file);
    
    if (success) {
        // Read converted WAV data back into memory
        FILE* wav_file = fopen(temp_wav_file, "rb");
        if (wav_file) {
            fseek(wav_file, 0, SEEK_END);
            long wav_size = ftell(wav_file);
            fseek(wav_file, 0, SEEK_SET);
            
            wav_data.resize(wav_size);
            if (fread(wav_data.data(), 1, wav_size, wav_file) == (size_t)wav_size) {
                printf("M4A to WAV memory conversion complete (%zu bytes)\n", wav_data.size());
            } else {
                success = false;
                printf("Failed to read converted WAV data\n");
            }
            fclose(wav_file);
        } else {
            success = false;
            printf("Failed to read converted WAV file\n");
        }
    }
    
    // Clean up temporary files
    DeleteFileA(temp_m4a_file);
    DeleteFileA(temp_wav_file);
    
    return success;
}

bool convertM4aToWav(const char* m4a_filename, const char* wav_filename) {
    if (!initializeMediaFoundation()) {
        return false;
    }
    
    // Convert filename to wide string
    std::wstring wide_filename = stringToWString(m4a_filename);
    
    HRESULT hr;
    IMFSourceReader* pReader = nullptr;
    
    // Create source reader from file
    hr = MFCreateSourceReaderFromURL(wide_filename.c_str(), nullptr, &pReader);
    if (FAILED(hr)) {
        printf("Cannot open M4A file: %s (Error: 0x%lx)\n", m4a_filename, hr);
        return false;
    }
    
    // Configure source reader for PCM output
    IMFMediaType* pType = nullptr;
    hr = MFCreateMediaType(&pType);
    if (FAILED(hr)) {
        printf("Failed to create media type\n");
        pReader->Release();
        return false;
    }
    
    hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (SUCCEEDED(hr)) {
        hr = pType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    }
    
    if (SUCCEEDED(hr)) {
        hr = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pType);
    }
    
    if (FAILED(hr)) {
        printf("Failed to configure source reader\n");
        pType->Release();
        pReader->Release();
        return false;
    }
    
    // Get audio format information
    IMFMediaType* pCurrentType = nullptr;
    hr = pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pCurrentType);
    if (FAILED(hr)) {
        printf("Failed to get current media type\n");
        pType->Release();
        pReader->Release();
        return false;
    }
    
    UINT32 sampleRate = 0;
    UINT32 channels = 0;
    UINT32 bitsPerSample = 16; // Default to 16-bit
    
    pCurrentType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
    pCurrentType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
    pCurrentType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample);
    
    printf("M4A: %d Hz, %d channels, %d bits per sample\n", sampleRate, channels, bitsPerSample);
    
    // Create WAV file
    FILE* wav_file = fopen(wav_filename, "wb");
    if (!wav_file) {
        printf("Cannot create WAV file: %s\n", wav_filename);
        pCurrentType->Release();
        pType->Release();
        pReader->Release();
        return false;
    }
    
    // Write initial WAV header (we'll update sizes later)
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
    int byte_rate = sampleRate * channels * bitsPerSample / 8;
    short block_align = channels * bitsPerSample / 8;
    
    fwrite(&fmt_chunk_size, 4, 1, wav_file);
    fwrite(&audio_format, 2, 1, wav_file);
    fwrite(&channels, 2, 1, wav_file);
    fwrite(&sampleRate, 4, 1, wav_file);
    fwrite(&byte_rate, 4, 1, wav_file);
    fwrite(&block_align, 2, 1, wav_file);
    fwrite(&bitsPerSample, 2, 1, wav_file);
    
    // data chunk header
    fwrite("data", 1, 4, wav_file);
    data_size_pos = ftell(wav_file);
    fwrite(&placeholder, 4, 1, wav_file); // Data size (will update later)
    
    long audio_data_start = ftell(wav_file);
    
    // Read and write audio data
    uint32_t total_bytes_written = 0;
    
    while (SUCCEEDED(hr)) {
        DWORD flags = 0;
        LONGLONG timestamp = 0;
        IMFSample* pSample = nullptr;
        
        hr = pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, 
                                nullptr, &flags, &timestamp, &pSample);
        
        if (FAILED(hr)) break;
        
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
            break;
        }
        
        if (pSample) {
            IMFMediaBuffer* pBuffer = nullptr;
            hr = pSample->ConvertToContiguousBuffer(&pBuffer);
            if (SUCCEEDED(hr)) {
                BYTE* pData = nullptr;
                DWORD maxLength = 0, currentLength = 0;
                
                hr = pBuffer->Lock(&pData, &maxLength, &currentLength);
                if (SUCCEEDED(hr)) {
                    fwrite(pData, 1, currentLength, wav_file);
                    total_bytes_written += currentLength;
                    pBuffer->Unlock();
                }
                pBuffer->Release();
            }
            pSample->Release();
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
    pCurrentType->Release();
    pType->Release();
    pReader->Release();
    
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

#endif // _WIN32
