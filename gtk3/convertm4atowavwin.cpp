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

bool convertM4aToVirtualWav(const std::vector<uint8_t>& m4a_data, const char* virtual_wav_filename) {
    if (!initializeMediaFoundation()) {
        return false;
    }
    
    // Create a temporary M4A file (Media Foundation still requires file access for M4A)
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
    
    // Convert filename to wide string
    std::wstring wide_filename = stringToWString(temp_m4a_file);
    
    HRESULT hr;
    IMFSourceReader* pReader = nullptr;
    
    // Create source reader from file
    hr = MFCreateSourceReaderFromURL(wide_filename.c_str(), nullptr, &pReader);
    if (FAILED(hr)) {
        printf("Cannot open M4A file: %s (Error: 0x%lx)\n", temp_m4a_file, hr);
        DeleteFileA(temp_m4a_file);
        return false;
    }
    
    // Configure source reader for PCM output
    IMFMediaType* pType = nullptr;
    hr = MFCreateMediaType(&pType);
    if (FAILED(hr)) {
        printf("Failed to create media type\n");
        pReader->Release();
        DeleteFileA(temp_m4a_file);
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
        DeleteFileA(temp_m4a_file);
        return false;
    }
    
    // Get audio format information
    IMFMediaType* pCurrentType = nullptr;
    hr = pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pCurrentType);
    if (FAILED(hr)) {
        printf("Failed to get current media type\n");
        pType->Release();
        pReader->Release();
        DeleteFileA(temp_m4a_file);
        return false;
    }
    
    UINT32 sampleRate = 0;
    UINT32 channels = 0;
    UINT32 bitsPerSample = 16; // Default to 16-bit
    
    pCurrentType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
    pCurrentType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
    pCurrentType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample);
    
    printf("M4A: %d Hz, %d channels, %d bits per sample\n", sampleRate, channels, bitsPerSample);
    
    // Create virtual WAV converter instead of physical file
    VirtualWAVConverter* wav_converter = virtual_wav_converter_init(virtual_wav_filename, sampleRate, channels);
    if (!wav_converter) {
        printf("Cannot create virtual WAV converter: %s\n", virtual_wav_filename);
        pCurrentType->Release();
        pType->Release();
        pReader->Release();
        DeleteFileA(temp_m4a_file);
        return false;
    }
    
    // Read and convert audio data directly to virtual file
    uint32_t total_samples_written = 0;
    
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
                    // Write PCM data directly to virtual file
                    size_t samples_in_buffer = currentLength / sizeof(int16_t);
                    if (!virtual_wav_converter_write(wav_converter, (int16_t*)pData, samples_in_buffer)) {
                        printf("Failed to write to virtual WAV file\n");
                        hr = E_FAIL;
                    } else {
                        total_samples_written += samples_in_buffer;
                    }
                    pBuffer->Unlock();
                }
                pBuffer->Release();
            }
            pSample->Release();
        }
    }
    
    // Finalize the virtual WAV file
    virtual_wav_converter_finish(wav_converter);
    virtual_wav_converter_free(wav_converter);
    
    // Cleanup
    pCurrentType->Release();
    pType->Release();
    pReader->Release();
    DeleteFileA(temp_m4a_file);
    
    printf("M4A conversion to virtual WAV complete: %d samples written\n", total_samples_written);
    return SUCCEEDED(hr);
}

bool convertM4aToWavInMemory(const std::vector<uint8_t>& m4a_data, std::vector<uint8_t>& wav_data) {
    // Generate a temporary virtual filename
    static int virtual_counter = 0;
    char virtual_filename[256];
    snprintf(virtual_filename, sizeof(virtual_filename), "temp_m4a_conversion_%d.wav", virtual_counter++);
    
    // Convert to virtual WAV file
    if (!convertM4aToVirtualWav(m4a_data, virtual_filename)) {
        return false;
    }
    
    // Read the virtual WAV file data back into the vector
    VirtualFile* vf = get_virtual_file(virtual_filename);
    if (!vf) {
        printf("Failed to retrieve converted virtual WAV file\n");
        return false;
    }
    
    // Copy data from virtual file to output vector
    wav_data.resize(vf->size);
    virtual_file_seek(vf, 0, SEEK_SET);
    size_t bytes_read = virtual_file_read(vf, wav_data.data(), vf->size);
    
    if (bytes_read != vf->size) {
        printf("Failed to read complete virtual WAV data\n");
        delete_virtual_file(virtual_filename);
        return false;
    }
    
    // Clean up temporary virtual file
    delete_virtual_file(virtual_filename);
    
    printf("M4A to WAV memory conversion complete (%zu bytes)\n", wav_data.size());
    return true;
}

bool convertM4aToWav(const char* m4a_filename, const char* wav_filename) {
    // Read M4A file into memory
    FILE* m4a_file = fopen(m4a_filename, "rb");
    if (!m4a_file) {
        printf("Cannot open M4A file: %s\n", m4a_filename);
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
    
    // Convert to virtual WAV first
    char virtual_filename[512];
    snprintf(virtual_filename, sizeof(virtual_filename), "temp_conversion_%s.wav", 
             strrchr(m4a_filename, '/') ? strrchr(m4a_filename, '/') + 1 : m4a_filename);
    
    if (!convertM4aToVirtualWav(m4a_data, virtual_filename)) {
        return false;
    }
    
    // Write virtual WAV to physical file
    VirtualFile* vf = get_virtual_file(virtual_filename);
    if (!vf) {
        printf("Failed to retrieve virtual WAV file\n");
        return false;
    }
    
    FILE* wav_file = fopen(wav_filename, "wb");
    if (!wav_file) {
        printf("Cannot create WAV file: %s\n", wav_filename);
        delete_virtual_file(virtual_filename);
        return false;
    }
    
    // Copy virtual file to physical file
    virtual_file_seek(vf, 0, SEEK_SET);
    char buffer[8192];
    size_t remaining = vf->size;
    
    while (remaining > 0) {
        size_t to_read = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
        size_t read = virtual_file_read(vf, buffer, to_read);
        if (read == 0) break;
        
        if (fwrite(buffer, 1, read, wav_file) != read) {
            printf("Failed to write WAV data\n");
            fclose(wav_file);
            delete_virtual_file(virtual_filename);
            return false;
        }
        remaining -= read;
    }
    
    fclose(wav_file);
    delete_virtual_file(virtual_filename);
    
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
    
    // Convert M4A to virtual WAV directly
    if (!convertM4aToVirtualWav(m4a_data, virtual_filename)) {
        printf("M4A to virtual WAV conversion failed\n");
        return false;
    }
    
    // Add to cache after successful conversion
    add_to_conversion_cache(&player->conversion_cache, filename, virtual_filename);
    
    printf("M4A conversion to virtual file complete\n");
    return true;
}

#endif // _WIN32
