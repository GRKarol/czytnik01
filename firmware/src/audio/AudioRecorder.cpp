// firmware/src/audio/AudioRecorder.cpp
#include "audio/AudioRecorder.h"

#include <SD_MMC.h>
#include <Wire.h>
#include <driver/i2s.h>
#include <esp_log.h>

#include "board/BoardConfig.h"

static const char* TAG = "AudioRecorder";

namespace {
constexpr i2s_port_t kI2sPort = I2S_NUM_0;

// ES8311 register addresses
constexpr uint8_t kEs8311ResetReg = 0x00;
constexpr uint8_t kEs8311ClkManagerReg01 = 0x01;
constexpr uint8_t kEs8311ClkManagerReg02 = 0x02;
constexpr uint8_t kEs8311ClkManagerReg03 = 0x03;
constexpr uint8_t kEs8311ClkManagerReg04 = 0x04;
constexpr uint8_t kEs8311ClkManagerReg05 = 0x05;
constexpr uint8_t kEs8311ClkManagerReg06 = 0x06;
constexpr uint8_t kEs8311ClkManagerReg07 = 0x07;
constexpr uint8_t kEs8311ClkManagerReg08 = 0x08;
constexpr uint8_t kEs8311SdPinReg09 = 0x09;
constexpr uint8_t kEs8311SdPoutReg0A = 0x0A;
constexpr uint8_t kEs8311SystemReg0B = 0x0B;
constexpr uint8_t kEs8311SystemReg0C = 0x0C;
constexpr uint8_t kEs8311SystemReg0D = 0x0D;
constexpr uint8_t kEs8311SystemReg0E = 0x0E;
constexpr uint8_t kEs8311SystemReg10 = 0x10;
constexpr uint8_t kEs8311SystemReg11 = 0x11;
constexpr uint8_t kEs8311SystemReg12 = 0x12;
constexpr uint8_t kEs8311SystemReg13 = 0x13;
constexpr uint8_t kEs8311SystemReg14 = 0x14;
constexpr uint8_t kEs8311AdcReg15 = 0x15;
constexpr uint8_t kEs8311AdcReg16 = 0x16;
constexpr uint8_t kEs8311AdcReg17 = 0x17;
constexpr uint8_t kEs8311AdcReg1B = 0x1B;
constexpr uint8_t kEs8311AdcReg1C = 0x1C;
constexpr uint8_t kEs8311DacReg31 = 0x31;
constexpr uint8_t kEs8311DacReg32 = 0x32;
constexpr uint8_t kEs8311DacReg37 = 0x37;
constexpr uint8_t kEs8311GpioReg44 = 0x44;
constexpr uint8_t kEs8311GpReg45 = 0x45;

constexpr uint8_t kIoConfigRegister = 0x03;
constexpr uint8_t kIoOutputRegister = 0x01;

}  // namespace

bool AudioRecorder::begin() {
    if (initialized_) return true;
    initialized_ = true;
    return true;
}

// ─── Recording ──────────────────────────────────────────────────────────────

bool AudioRecorder::startRecording(const char* absolutePath) {
    if (recording_ || playing_) {
        ESP_LOGW(TAG, "Cannot start recording — already busy");
        return false;
    }

    currentFilePath_ = absolutePath;
    stopRequested_ = false;

    if (!enableAudioRail()) {
        ESP_LOGE(TAG, "Failed to enable audio rail");
        return false;
    }

    delay(15);

    if (!configureCodecForRecording()) {
        ESP_LOGE(TAG, "Failed to configure codec for recording");
        return false;
    }

    if (!configureI2sForRecording()) {
        ESP_LOGE(TAG, "Failed to configure I2S for recording");
        return false;
    }

    recording_ = true;
    recordStartMs_ = millis();

    BaseType_t result = xTaskCreatePinnedToCore(
        recordTaskEntry, "rec", kRecordTaskStackSize, this, 3, &recordTask_, 1);

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create record task");
        recording_ = false;
        deinitI2s();
        return false;
    }

    ESP_LOGI(TAG, "Recording started: %s", absolutePath);
    return true;
}

bool AudioRecorder::stopRecording() {
    if (!recording_) return false;

    stopRequested_ = true;

    // Wait for task to finish (max 2s)
    uint32_t waitStart = millis();
    while (recording_ && (millis() - waitStart) < 2000) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (recording_) {
        // Force kill
        if (recordTask_) {
            vTaskDelete(recordTask_);
            recordTask_ = nullptr;
        }
        recording_ = false;
    }

    deinitI2s();
    ESP_LOGI(TAG, "Recording stopped");
    return true;
}

bool AudioRecorder::isRecording() const {
    return recording_;
}

uint32_t AudioRecorder::recordingElapsedMs() const {
    if (!recording_) return 0;
    return millis() - recordStartMs_;
}

// ─── Playback ───────────────────────────────────────────────────────────────

bool AudioRecorder::startPlayback(const char* absolutePath) {
    if (recording_ || playing_) {
        ESP_LOGW(TAG, "Cannot start playback — already busy");
        return false;
    }

    // Verify file exists and read WAV header
    File file = SD_MMC.open(absolutePath, FILE_READ);
    if (!file) {
        ESP_LOGE(TAG, "Playback file not found: %s", absolutePath);
        return false;
    }

    WavHeader hdr;
    if (file.read(reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr)) != sizeof(hdr)) {
        file.close();
        ESP_LOGE(TAG, "Failed to read WAV header");
        return false;
    }

    // Validate WAV
    if (memcmp(hdr.riff, "RIFF", 4) != 0 || memcmp(hdr.wave, "WAVE", 4) != 0) {
        file.close();
        ESP_LOGE(TAG, "Invalid WAV file");
        return false;
    }

    uint32_t totalSamples = hdr.dataSize / (hdr.bitsPerSample / 8) / hdr.numChannels;
    playbackTotalMs_ = (totalSamples * 1000UL) / hdr.sampleRate;

    file.close();

    currentFilePath_ = absolutePath;
    stopRequested_ = false;

    if (!enableAudioRail()) {
        ESP_LOGE(TAG, "Failed to enable audio rail for playback");
        return false;
    }

    delay(15);

    if (!configureCodecForPlayback()) {
        ESP_LOGE(TAG, "Failed to configure codec for playback");
        return false;
    }

    if (!configureI2sForPlayback()) {
        ESP_LOGE(TAG, "Failed to configure I2S for playback");
        return false;
    }

    playing_ = true;
    playbackStartMs_ = millis();

    BaseType_t result = xTaskCreatePinnedToCore(
        playbackTaskEntry, "play", kPlaybackTaskStackSize, this, 3, &playbackTask_, 1);

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create playback task");
        playing_ = false;
        deinitI2s();
        return false;
    }

    ESP_LOGI(TAG, "Playback started: %s", absolutePath);
    return true;
}

bool AudioRecorder::stopPlayback() {
    if (!playing_) return false;

    stopRequested_ = true;

    uint32_t waitStart = millis();
    while (playing_ && (millis() - waitStart) < 2000) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (playing_) {
        if (playbackTask_) {
            vTaskDelete(playbackTask_);
            playbackTask_ = nullptr;
        }
        playing_ = false;
    }

    deinitI2s();
    ESP_LOGI(TAG, "Playback stopped");
    return true;
}

bool AudioRecorder::isPlaying() const {
    return playing_;
}

uint32_t AudioRecorder::playbackElapsedMs() const {
    if (!playing_) return 0;
    return millis() - playbackStartMs_;
}

uint32_t AudioRecorder::playbackTotalMs() const {
    return playbackTotalMs_;
}

// ─── I2S Configuration ──────────────────────────────────────────────────────

bool AudioRecorder::configureI2sForRecording() {
    i2s_config_t config = {};
    config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
    config.sample_rate = kSampleRate;
    config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    config.intr_alloc_flags = 0;
    config.dma_buf_count = 4;
    config.dma_buf_len = 256;
    config.use_apll = false;
    config.tx_desc_auto_clear = false;
    config.fixed_mclk = 0;
    config.mclk_multiple = I2S_MCLK_MULTIPLE_256;

    esp_err_t err = i2s_driver_install(kI2sPort, &config, 0, nullptr);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2S driver install (RX) failed: %s", esp_err_to_name(err));
        return false;
    }

    i2s_pin_config_t pins = {};
    pins.mck_io_num = BoardConfig::PIN_AUDIO_MCLK;
    pins.bck_io_num = BoardConfig::PIN_AUDIO_BCLK;
    pins.ws_io_num = BoardConfig::PIN_AUDIO_WS;
    pins.data_out_num = I2S_PIN_NO_CHANGE;
    pins.data_in_num = BoardConfig::PIN_AUDIO_DIN;

    err = i2s_set_pin(kI2sPort, &pins);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2S set pins (RX) failed: %s", esp_err_to_name(err));
        i2s_driver_uninstall(kI2sPort);
        return false;
    }

    i2s_set_clk(kI2sPort, kSampleRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
    return true;
}

bool AudioRecorder::configureI2sForPlayback() {
    i2s_config_t config = {};
    config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
    config.sample_rate = kSampleRate;
    config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    config.intr_alloc_flags = 0;
    config.dma_buf_count = 4;
    config.dma_buf_len = 256;
    config.use_apll = false;
    config.tx_desc_auto_clear = true;
    config.fixed_mclk = 0;
    config.mclk_multiple = I2S_MCLK_MULTIPLE_256;

    esp_err_t err = i2s_driver_install(kI2sPort, &config, 0, nullptr);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2S driver install (TX) failed: %s", esp_err_to_name(err));
        return false;
    }

    i2s_pin_config_t pins = {};
    pins.mck_io_num = BoardConfig::PIN_AUDIO_MCLK;
    pins.bck_io_num = BoardConfig::PIN_AUDIO_BCLK;
    pins.ws_io_num = BoardConfig::PIN_AUDIO_WS;
    pins.data_out_num = BoardConfig::PIN_AUDIO_DOUT;
    pins.data_in_num = I2S_PIN_NO_CHANGE;

    err = i2s_set_pin(kI2sPort, &pins);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2S set pins (TX) failed: %s", esp_err_to_name(err));
        i2s_driver_uninstall(kI2sPort);
        return false;
    }

    i2s_set_clk(kI2sPort, kSampleRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
    i2s_zero_dma_buffer(kI2sPort);
    return true;
}

void AudioRecorder::deinitI2s() {
    i2s_driver_uninstall(kI2sPort);
}

// ─── Codec Configuration ────────────────────────────────────────────────────

bool AudioRecorder::configureCodecForRecording() {
    uint8_t reg = 0;

    // Reset codec
    if (!writeCodecRegister(kEs8311ResetReg, 0x80)) return false;
    delay(5);
    if (!readCodecRegister(kEs8311ResetReg, reg)) return false;
    if (!writeCodecRegister(kEs8311ResetReg, static_cast<uint8_t>(reg & 0xBF))) return false;

    // Clock setup — MCLK from I2S master, 256x oversampling
    if (!writeCodecRegister(kEs8311ClkManagerReg01, 0x3F)) return false;
    if (!writeCodecRegister(kEs8311ClkManagerReg02, 0x00)) return false;
    if (!writeCodecRegister(kEs8311ClkManagerReg03, 0x10)) return false;
    if (!writeCodecRegister(kEs8311ClkManagerReg04, 0x10)) return false;
    if (!writeCodecRegister(kEs8311ClkManagerReg05, 0x00)) return false;
    if (!readCodecRegister(kEs8311ClkManagerReg06, reg)) return false;
    if (!writeCodecRegister(kEs8311ClkManagerReg06, static_cast<uint8_t>((reg & 0xE0) | 0x03))) return false;
    if (!readCodecRegister(kEs8311ClkManagerReg07, reg)) return false;
    if (!writeCodecRegister(kEs8311ClkManagerReg07, static_cast<uint8_t>(reg & 0xC0))) return false;
    if (!writeCodecRegister(kEs8311ClkManagerReg08, 0xFF)) return false;

    // ADC serial data format: 16-bit I2S
    if (!readCodecRegister(kEs8311SdPoutReg0A, reg)) return false;
    reg = static_cast<uint8_t>((reg & 0xE0) | 0x0C);  // 16-bit
    reg |= (1U << 6);  // ADC output enable on SDOUT
    if (!writeCodecRegister(kEs8311SdPoutReg0A, reg)) return false;

    // System configuration — power up ADC
    if (!writeCodecRegister(kEs8311SystemReg0B, 0x00)) return false;
    if (!writeCodecRegister(kEs8311SystemReg0C, 0x00)) return false;
    if (!writeCodecRegister(kEs8311SystemReg10, 0x1F)) return false;
    if (!writeCodecRegister(kEs8311SystemReg11, 0x7F)) return false;
    if (!writeCodecRegister(kEs8311SystemReg0D, 0x01)) return false;
    if (!writeCodecRegister(kEs8311SystemReg0E, 0x02)) return false;
    if (!writeCodecRegister(kEs8311SystemReg12, 0x00)) return false;
    if (!writeCodecRegister(kEs8311SystemReg13, 0x10)) return false;
    if (!writeCodecRegister(kEs8311SystemReg14, 0x1A)) return false;

    // ADC configuration — microphone input with PGA gain
    if (!writeCodecRegister(kEs8311AdcReg15, 0x40)) return false;  // ADC power on
    if (!writeCodecRegister(kEs8311AdcReg16, 0x24)) return false;  // MIC input select
    if (!writeCodecRegister(kEs8311AdcReg17, 0xBF)) return false;  // ADC enable, HPF on
    if (!writeCodecRegister(kEs8311AdcReg1B, 0x0A)) return false;  // MIC PGA gain
    if (!writeCodecRegister(kEs8311AdcReg1C, 0x6A)) return false;  // ADC volume

    // GPIO configuration
    if (!writeCodecRegister(kEs8311GpioReg44, 0x58)) return false;
    if (!writeCodecRegister(kEs8311GpReg45, 0x00)) return false;

    ESP_LOGI(TAG, "Codec configured for recording");
    return true;
}

bool AudioRecorder::configureCodecForPlayback() {
    uint8_t reg = 0;

    // Reset codec
    if (!writeCodecRegister(kEs8311ResetReg, 0x80)) return false;
    delay(5);
    if (!readCodecRegister(kEs8311ResetReg, reg)) return false;
    if (!writeCodecRegister(kEs8311ResetReg, static_cast<uint8_t>(reg & 0xBF))) return false;

    // Clock setup
    if (!writeCodecRegister(kEs8311ClkManagerReg01, 0x3F)) return false;
    if (!writeCodecRegister(kEs8311ClkManagerReg02, 0x00)) return false;
    if (!writeCodecRegister(kEs8311ClkManagerReg03, 0x10)) return false;
    if (!writeCodecRegister(kEs8311ClkManagerReg04, 0x10)) return false;
    if (!writeCodecRegister(kEs8311ClkManagerReg05, 0x00)) return false;
    if (!readCodecRegister(kEs8311ClkManagerReg06, reg)) return false;
    if (!writeCodecRegister(kEs8311ClkManagerReg06, static_cast<uint8_t>((reg & 0xE0) | 0x03))) return false;
    if (!readCodecRegister(kEs8311ClkManagerReg07, reg)) return false;
    if (!writeCodecRegister(kEs8311ClkManagerReg07, static_cast<uint8_t>(reg & 0xC0))) return false;
    if (!writeCodecRegister(kEs8311ClkManagerReg08, 0xFF)) return false;

    // DAC serial data format: 16-bit I2S
    if (!readCodecRegister(kEs8311SdPinReg09, reg)) return false;
    reg = static_cast<uint8_t>((reg & 0xE0) | 0x0C);
    reg &= ~(1U << 6);  // DAC input enable on SDIN
    if (!writeCodecRegister(kEs8311SdPinReg09, reg)) return false;

    // System power up — DAC path
    if (!writeCodecRegister(kEs8311SystemReg0B, 0x00)) return false;
    if (!writeCodecRegister(kEs8311SystemReg0C, 0x00)) return false;
    if (!writeCodecRegister(kEs8311SystemReg10, 0x1F)) return false;
    if (!writeCodecRegister(kEs8311SystemReg11, 0x7F)) return false;
    if (!writeCodecRegister(kEs8311SystemReg0D, 0x01)) return false;
    if (!writeCodecRegister(kEs8311SystemReg0E, 0x02)) return false;
    if (!writeCodecRegister(kEs8311SystemReg12, 0x00)) return false;
    if (!writeCodecRegister(kEs8311SystemReg13, 0x10)) return false;
    if (!writeCodecRegister(kEs8311SystemReg14, 0x1A)) return false;

    // DAC configuration
    if (!writeCodecRegister(kEs8311DacReg31, 0x00)) return false;  // unmute DAC
    if (!writeCodecRegister(kEs8311DacReg32, 0xF0)) return false;  // DAC volume
    if (!writeCodecRegister(kEs8311DacReg37, 0x08)) return false;

    // GPIO
    if (!writeCodecRegister(kEs8311GpioReg44, 0x58)) return false;
    if (!writeCodecRegister(kEs8311GpReg45, 0x00)) return false;

    ESP_LOGI(TAG, "Codec configured for playback");
    return true;
}

// ─── Audio Rail ─────────────────────────────────────────────────────────────

bool AudioRecorder::enableAudioRail() {
    uint8_t direction = 0xFF;
    uint8_t output = 0xFF;

    Wire1.beginTransmission(BoardConfig::TCA9554_ADDRESS);
    Wire1.write(kIoConfigRegister);
    if (Wire1.endTransmission(false) != 0) return false;
    if (Wire1.requestFrom(static_cast<int>(BoardConfig::TCA9554_ADDRESS), 1, 1) != 1) return false;
    direction = Wire1.read();

    Wire1.beginTransmission(BoardConfig::TCA9554_ADDRESS);
    Wire1.write(kIoOutputRegister);
    if (Wire1.endTransmission(false) != 0) return false;
    if (Wire1.requestFrom(static_cast<int>(BoardConfig::TCA9554_ADDRESS), 1, 1) != 1) return false;
    output = Wire1.read();

    const uint8_t mask = static_cast<uint8_t>(1U << BoardConfig::TCA9554_PIN_AUDIO_ENABLE);
    output |= mask;
    direction &= static_cast<uint8_t>(~mask);

    Wire1.beginTransmission(BoardConfig::TCA9554_ADDRESS);
    Wire1.write(kIoOutputRegister);
    Wire1.write(output);
    if (Wire1.endTransmission(true) != 0) return false;

    Wire1.beginTransmission(BoardConfig::TCA9554_ADDRESS);
    Wire1.write(kIoConfigRegister);
    Wire1.write(direction);
    return Wire1.endTransmission(true) == 0;
}

// ─── Codec Register Access ──────────────────────────────────────────────────

bool AudioRecorder::readCodecRegister(uint8_t reg, uint8_t& value) {
    Wire1.beginTransmission(BoardConfig::ES8311_ADDRESS);
    Wire1.write(reg);
    if (Wire1.endTransmission(false) != 0) return false;
    if (Wire1.requestFrom(static_cast<int>(BoardConfig::ES8311_ADDRESS), 1, 1) != 1) return false;
    value = Wire1.read();
    return true;
}

bool AudioRecorder::writeCodecRegister(uint8_t reg, uint8_t value) {
    Wire1.beginTransmission(BoardConfig::ES8311_ADDRESS);
    Wire1.write(reg);
    Wire1.write(value);
    return Wire1.endTransmission(true) == 0;
}

// ─── WAV Header ─────────────────────────────────────────────────────────────

static bool writeWavHeader(File& file, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels) {
    AudioRecorder::WavHeader hdr;
    hdr.sampleRate = sampleRate;
    hdr.numChannels = numChannels;
    hdr.bitsPerSample = bitsPerSample;
    hdr.byteRate = sampleRate * numChannels * (bitsPerSample / 8);
    hdr.blockAlign = numChannels * (bitsPerSample / 8);
    // Write placeholder header (fileSize and dataSize will be filled on stop)
    return file.write(reinterpret_cast<const uint8_t*>(&hdr), sizeof(hdr)) == sizeof(hdr);
}

static bool finalizeWavHeader(File& file, uint32_t dataBytes, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels) {
    AudioRecorder::WavHeader hdr;
    hdr.sampleRate = sampleRate;
    hdr.numChannels = numChannels;
    hdr.bitsPerSample = bitsPerSample;
    hdr.byteRate = sampleRate * numChannels * (bitsPerSample / 8);
    hdr.blockAlign = numChannels * (bitsPerSample / 8);
    hdr.dataSize = dataBytes;
    hdr.fileSize = sizeof(AudioRecorder::WavHeader) - 8 + dataBytes;

    file.seek(0);
    return file.write(reinterpret_cast<const uint8_t*>(&hdr), sizeof(hdr)) == sizeof(hdr);
}

// ─── Record Task ────────────────────────────────────────────────────────────

void AudioRecorder::recordTaskEntry(void* param) {
    static_cast<AudioRecorder*>(param)->recordTaskLoop();
    vTaskDelete(nullptr);
}

void AudioRecorder::recordTaskLoop() {
    File file = SD_MMC.open(currentFilePath_, FILE_WRITE);
    if (!file) {
        ESP_LOGE(TAG, "Cannot create recording file: %s", currentFilePath_.c_str());
        recording_ = false;
        recordTask_ = nullptr;
        return;
    }

    writeWavHeader(file, kSampleRate, kBitsPerSample, kChannels);

    uint8_t buffer[kRecordBufferSize];
    int16_t monoBuffer[kRecordBufferSize / 4];  // stereo → mono
    uint32_t totalDataBytes = 0;

    while (!stopRequested_) {
        // Check max duration
        if ((millis() - recordStartMs_) >= kMaxRecordingMs) {
            ESP_LOGI(TAG, "Max recording duration reached");
            break;
        }

        size_t bytesRead = 0;
        esp_err_t err = i2s_read(kI2sPort, buffer, kRecordBufferSize, &bytesRead, pdMS_TO_TICKS(100));

        if (err != ESP_OK || bytesRead == 0) {
            continue;
        }

        // Convert stereo 16-bit to mono (take left channel only)
        size_t stereoSamples = bytesRead / 4;  // 2 bytes * 2 channels per sample
        const int16_t* stereoData = reinterpret_cast<const int16_t*>(buffer);
        for (size_t i = 0; i < stereoSamples; i++) {
            monoBuffer[i] = stereoData[i * 2];  // left channel
        }

        size_t monoBytes = stereoSamples * 2;
        size_t written = file.write(reinterpret_cast<const uint8_t*>(monoBuffer), monoBytes);
        if (written != monoBytes) {
            ESP_LOGE(TAG, "SD write error during recording");
            break;
        }

        totalDataBytes += written;
    }

    // Finalize WAV header
    finalizeWavHeader(file, totalDataBytes, kSampleRate, kBitsPerSample, kChannels);
    file.close();

    recording_ = false;
    recordTask_ = nullptr;
    ESP_LOGI(TAG, "Recording complete: %u bytes", totalDataBytes);
}

// ─── Playback Task ──────────────────────────────────────────────────────────

void AudioRecorder::playbackTaskEntry(void* param) {
    static_cast<AudioRecorder*>(param)->playbackTaskLoop();
    vTaskDelete(nullptr);
}

void AudioRecorder::playbackTaskLoop() {
    File file = SD_MMC.open(currentFilePath_, FILE_READ);
    if (!file) {
        ESP_LOGE(TAG, "Cannot open playback file: %s", currentFilePath_.c_str());
        playing_ = false;
        playbackTask_ = nullptr;
        return;
    }

    // Skip WAV header
    file.seek(sizeof(WavHeader));

    uint8_t monoBuffer[kPlaybackBufferSize / 2];
    int16_t stereoBuffer[kPlaybackBufferSize / 2];  // mono → stereo

    while (!stopRequested_ && file.available()) {
        size_t monoBytes = file.read(monoBuffer, sizeof(monoBuffer));
        if (monoBytes == 0) break;

        // Convert mono 16-bit to stereo (duplicate to both channels)
        size_t monoSamples = monoBytes / 2;
        const int16_t* monoData = reinterpret_cast<const int16_t*>(monoBuffer);
        for (size_t i = 0; i < monoSamples; i++) {
            stereoBuffer[i * 2] = monoData[i];      // left
            stereoBuffer[i * 2 + 1] = monoData[i];  // right
        }

        size_t stereoBytes = monoSamples * 4;
        size_t bytesWritten = 0;
        i2s_write(kI2sPort, stereoBuffer, stereoBytes, &bytesWritten, pdMS_TO_TICKS(250));
    }

    file.close();
    playing_ = false;
    playbackTask_ = nullptr;
    ESP_LOGI(TAG, "Playback complete");
}
