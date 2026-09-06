// firmware/src/audio/AudioRecorderDiag.cpp
//
// Throwaway diagnostic build (enabled only by -DAUDIO_DIAG=1, see the
// waveshare_esp32s3_audio_diag env in platformio.ini).
//
// ── What round 1 established ────────────────────────────────────────────────
// The first sweep proved the microphone is alive: real, varying samples at
// roughly -34 dBFS arrive on DIN in every 16-bit Philips variant. But the
// data is not continuous. In all ten variants — 16-bit and 32-bit, Philips
// and MSB, MIC1+3 and MIC1+2, TDM and plain stereo, codec-first and
// I2S-first — exactly one LRCK frame in every sixteen carries data and the
// other fifteen are zero-filled. The ratio is identical across variants when
// measured in *frames*, not in bytes or in time, which means the ES7210's ADC
// is producing samples at fs/16 and the framing/format knobs we spent nine
// attempts on were never the variable.
//
// ── Where the factor of 16 comes from ───────────────────────────────────────
// Reading the ES7210 registers back after each variant showed two values that
// do not match what was written, and they are exactly the two that set the
// ADC's clock divider chain:
//
//     reg 0x02 MAINCLK  written 0xC1  read back 0x81   (bit6, clock doubler)
//     reg 0x07 OSR      written 0x20  read back 0x00   (oversampling ratio)
//
// A doubler that never switched on is 2x too slow. An OSR field holding 0
// instead of 32 is 8x too slow. 2 x 8 = 16, which is precisely the observed
// deficit. Every other register with more than one bit set came back the same
// way — 0x1C read as 0x18, 0x70 as 0x60, 0x30 as 0x20, 0x60 as 0x40, 0x43 as
// 0x03 — i.e. a bit survives the round trip only when the adjacent bit is
// also set. Single-bit values (0x20, 0x08, 0x02) vanished entirely. That is
// the signature of an I2C byte whose 0->1 transitions are not settling in
// time, and Wire1 on this board runs at 300 kHz (BoardConfig.cpp:193).
//
// ── What this build tests ───────────────────────────────────────────────────
// 1. Whether the corruption is bus speed: the same register writes repeated
//    at 300/200/100/50 kHz, read back and compared byte for byte. The ES7210
//    chip-ID registers are read at each speed too — they returned 0xFF in
//    round 1, and a correct 0x72/0x10 at a lower speed would settle it.
// 2. Whether fixing the two clock registers fixes the ADC rate, measured as
//    the *stride*: the number of LRCK frames between consecutive frames that
//    carry data. Round 1 measured a stride of 16 everywhere. A working codec
//    gives a stride of 1. Variants that halve or double the divider on
//    purpose (OSR 0x10, OSR 0x02, doubler off) calibrate the relationship
//    rather than assuming it.
// 3. If any variant reaches a stride of 1, this build stops diagnosing and
//    records four real seconds of audio with that configuration, straight to
//    the file the Dictaphone expects — so a working configuration can be
//    heard on the same flash that found it.
#if AUDIO_DIAG

#include "audio/AudioRecorder.h"

#include <SD_MMC.h>
#include <Wire.h>
#include <driver/i2s.h>
#include <esp_log.h>
#include <stdarg.h>

#include "board/BoardConfig.h"

static const char* DTAG = "AudioDiag";

namespace {

constexpr i2s_port_t kPort = I2S_NUM_0;
constexpr uint32_t kRate = 16000;
constexpr size_t kCaptureBytes = 8192;
constexpr uint32_t kNormalI2cHz = 300000;

// ES7210 registers — redeclared here rather than shared with
// AudioRecorder.cpp's anonymous namespace, so this diagnostic file stays
// self-contained and can be deleted in one piece afterwards.
constexpr uint8_t kReset00 = 0x00;
constexpr uint8_t kClockOff01 = 0x01;
constexpr uint8_t kMainClk02 = 0x02;
constexpr uint8_t kPowerDown06 = 0x06;
constexpr uint8_t kOsr07 = 0x07;
constexpr uint8_t kModeCfg08 = 0x08;
constexpr uint8_t kTime09 = 0x09;
constexpr uint8_t kTime0A = 0x0A;
constexpr uint8_t kSdp1_11 = 0x11;
constexpr uint8_t kSdp2_12 = 0x12;
constexpr uint8_t kHpf20 = 0x20;
constexpr uint8_t kHpf21 = 0x21;
constexpr uint8_t kHpf22 = 0x22;
constexpr uint8_t kHpf23 = 0x23;
constexpr uint8_t kAnalog40 = 0x40;
constexpr uint8_t kBias41 = 0x41;
constexpr uint8_t kBias42 = 0x42;
constexpr uint8_t kGain43 = 0x43;   // 0x43..0x46 = MIC1..MIC4 gain
constexpr uint8_t kPower47 = 0x47;  // 0x47..0x4A = MIC1..MIC4 power
constexpr uint8_t kMic12Power4B = 0x4B;
constexpr uint8_t kMic34Power4C = 0x4C;

constexpr uint8_t kGain345db = 0x1C;  // PGA enable (0x10) | gain step 12

// One candidate configuration. The format/pairing axes are gone — round 1
// showed they make no difference to the stride — and the whole sweep now
// varies only the I2C bus speed and the two clock-divider registers.
struct Variant {
    const char* name;
    uint32_t i2cHz;
    uint8_t reg02;    // MAINCLK: adc_div | doubler<<6 | dll<<7
    uint8_t reg07;    // OSR
    uint8_t micMask;  // bit0=MIC1 .. bit3=MIC4
    uint8_t sdp12;    // 0x00 = plain framing, 0x02 = TDM framing
    bool espTdm4;     // true = 4-slot TDM on the ESP32 side
};

const Variant kVariants[] = {
    // Round 1's variant A, unchanged, so this run's numbers can be compared
    // against the previous ones on equal terms.
    {"1_base_i2c300", 300000, 0xC1, 0x20, 0b0101, 0x00, false},
    // The same thing at progressively slower bus speeds. If the corruption is
    // rise-time, one of these writes the registers correctly and the stride
    // drops from 16 to 1 without touching anything else.
    {"2_i2c200", 200000, 0xC1, 0x20, 0b0101, 0x00, false},
    {"3_i2c100", 100000, 0xC1, 0x20, 0b0101, 0x00, false},
    {"4_i2c50", 50000, 0xC1, 0x20, 0b0101, 0x00, false},
    // Pre-compensated values at the original speed: under the observed
    // corruption rule (a bit survives only if its neighbour is set) writing
    // 0xE1 lands as 0xC1 and writing 0x30 lands as 0x20. If this variant
    // works while variant 1 does not, the corruption is real and the rule is
    // exactly right, which also means every read-modify-write in the normal
    // driver has been writing garbage.
    {"5_precomp_i2c300", 300000, 0xE1, 0x30, 0b0101, 0x00, false},
    // Calibration probes. These deliberately move the divider by known
    // factors so the stride can be *measured* against the model instead of
    // assumed: halving OSR should halve the stride, dropping the doubler
    // should double it. If the stride does not move, the divider chain is not
    // where the factor of 16 lives and the model is wrong.
    {"6_i2c100_osr10", 100000, 0xC1, 0x10, 0b0101, 0x00, false},
    {"7_i2c100_osr02", 100000, 0xC1, 0x02, 0b0101, 0x00, false},
    {"8_i2c100_nodoubler", 100000, 0x81, 0x20, 0b0101, 0x00, false},
    // The reference firmware's own framing (4 mics, codec TDM, 4 ESP slots)
    // at the corrected bus speed — the one combination round 1 never had a
    // chance to judge, because the clock was wrong underneath it.
    {"9_i2c100_tdm4", 100000, 0xC1, 0x20, 0b1111, 0x02, true},
};

void rp(String& out, const char* fmt, ...) {
    char buf[224];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    out += buf;
    out += "\r\n";
}

// How many LRCK frames apart the frames carrying data are. 1 means the codec
// is producing a sample every frame, which is the whole goal; round 1
// measured 16 in every variant.
struct Capture {
    uint32_t frames = 0;
    uint32_t liveFrames = 0;
    uint32_t stride = 0;     // most common gap between live frames
    uint32_t maxZeroRun = 0;  // longest run of consecutive silent frames
    int16_t peak = 0;
};

Capture analyse(const uint8_t* buf, size_t bytes, uint32_t wordsPerFrame) {
    Capture c;
    const int16_t* w = reinterpret_cast<const int16_t*>(buf);
    const uint32_t words = bytes / 2;
    c.frames = words / wordsPerFrame;

    uint32_t gapHist[65] = {0};
    uint32_t lastLive = 0;
    bool haveLast = false;
    uint32_t zeroRun = 0;

    for (uint32_t f = 0; f < c.frames; f++) {
        bool live = false;
        for (uint32_t k = 0; k < wordsPerFrame; k++) {
            const int16_t v = w[f * wordsPerFrame + k];
            if (v != 0 && v != -1) live = true;
            const int16_t mag = (v < 0) ? static_cast<int16_t>(-(v + 1)) : v;
            if (mag > c.peak) c.peak = mag;
        }
        if (live) {
            c.liveFrames++;
            if (haveLast) {
                const uint32_t gap = f - lastLive;
                if (gap <= 64) gapHist[gap]++;
            }
            lastLive = f;
            haveLast = true;
            zeroRun = 0;
        } else {
            zeroRun++;
            if (zeroRun > c.maxZeroRun) c.maxZeroRun = zeroRun;
        }
    }

    uint32_t best = 0;
    for (uint32_t g = 1; g <= 64; g++) {
        if (gapHist[g] > best) {
            best = gapHist[g];
            c.stride = g;
        }
    }
    return c;
}

}  // namespace

void AudioRecorder::runDiagnostics() {
    String dir = currentFilePath_;
    int slash = dir.lastIndexOf('/');
    dir = (slash > 0) ? dir.substring(0, slash) : String("/");

    String rpt;
    rpt.reserve(16384);
    rp(rpt, "=== ES7210 clock / I2C integrity sweep (round 2) ===");
    rp(rpt, "build: %s %s", __DATE__, __TIME__);
    rp(rpt, "dir: %s", dir.c_str());
    rp(rpt, "round 1 result: real audio present, but stride 16 in every variant");

    if (!enableAudioRail()) {
        rp(rpt, "FATAL: enableAudioRail() failed (TCA9554 not responding)");
    } else {
        rp(rpt, "audio rail: enabled");
    }
    delay(20);

    // ── Test 1: is the register corruption a bus-speed problem? ─────────────
    // Writes a set of patterns to MIC1_GAIN (harmless, fully rewritten later)
    // at four bus speeds and reports what comes back. 0x20 and 0x30 are the
    // OSR values that matter; 0xC1 and 0xE1 are the MAINCLK ones; 0xAA/0x55
    // are the worst case for a line that cannot settle.
    rp(rpt, "");
    rp(rpt, "--- I2C write/read integrity (reg 0x43) ---");
    {
        static const uint8_t kPatterns[] = {0xAA, 0x55, 0x20, 0x30, 0xC1, 0xE1, 0x1C, 0x0F};
        static const uint32_t kSpeeds[] = {300000, 200000, 100000, 50000};
        for (uint32_t s = 0; s < sizeof(kSpeeds) / sizeof(kSpeeds[0]); s++) {
            Wire1.setClock(kSpeeds[s]);
            delay(5);
            String line;
            uint32_t bad = 0;
            for (uint32_t p = 0; p < sizeof(kPatterns) / sizeof(kPatterns[0]); p++) {
                uint8_t got = 0;
                char b[16];
                if (writeEs7210Register(kGain43, kPatterns[p]) &&
                    readEs7210Register(kGain43, got)) {
                    snprintf(b, sizeof(b), "%02X>%02X ", kPatterns[p], got);
                    if (got != kPatterns[p]) bad++;
                } else {
                    snprintf(b, sizeof(b), "%02X>ERR ", kPatterns[p]);
                    bad++;
                }
                line += b;
            }
            uint8_t idA = 0, idB = 0;
            readEs7210Register(0xFD, idA);
            readEs7210Register(0xFE, idB);
            rp(rpt, "%6u Hz: %s | mismatched %u/8 | chipid FD=%02X FE=%02X (want 72/10)",
               static_cast<unsigned>(kSpeeds[s]), line.c_str(), static_cast<unsigned>(bad), idA,
               idB);
        }
        Wire1.setClock(kNormalI2cHz);
    }

    uint8_t* buf = static_cast<uint8_t*>(malloc(kCaptureBytes));
    if (!buf) {
        rp(rpt, "FATAL: cannot allocate capture buffer");
    }

    // ── Test 2: does fixing the clock registers fix the ADC rate? ───────────
    rp(rpt, "");
    rp(rpt, "--- capture sweep (stride 16 = broken, stride 1 = fixed) ---");

    int bestIdx = -1;
    uint32_t bestStride = 0xFFFFFFFF;
    int16_t bestPeak = 0;

    for (size_t vi = 0; buf && vi < sizeof(kVariants) / sizeof(kVariants[0]); vi++) {
        const Variant& v = kVariants[vi];

        i2s_driver_uninstall(kPort);
        delay(10);
        Wire1.setClock(v.i2cHz);
        delay(5);

        bool cok = configureEs7210(v.reg02, v.reg07, v.micMask, v.sdp12);
        bool i2sOk = installDiagI2s(v.espTdm4);

        if (!i2sOk) {
            rp(rpt, "%-20s i2c=%6u  I2S INSTALL FAILED", v.name,
               static_cast<unsigned>(v.i2cHz));
            continue;
        }

        // Read the two registers that this whole run is about, so the numbers
        // below can be attributed to a value the chip actually holds rather
        // than to the value we asked for.
        uint8_t got02 = 0, got07 = 0, got11 = 0, got12 = 0;
        readEs7210Register(kMainClk02, got02);
        readEs7210Register(kOsr07, got07);
        readEs7210Register(kSdp1_11, got11);
        readEs7210Register(kSdp2_12, got12);

        // Let the ADC settle, then drop whatever the DMA ring held from
        // before it did.
        delay(250);
        {
            size_t junk = 0;
            for (int i = 0; i < 4; i++) i2s_read(kPort, buf, kCaptureBytes, &junk, pdMS_TO_TICKS(100));
        }

        size_t total = 0;
        esp_err_t lastErr = ESP_OK;
        uint32_t deadline = millis() + 2000;
        while (total < kCaptureBytes && millis() < deadline) {
            size_t got = 0;
            lastErr = i2s_read(kPort, buf + total, kCaptureBytes - total, &got, pdMS_TO_TICKS(200));
            if (lastErr != ESP_OK) break;
            if (got == 0) continue;
            total += got;
        }

        const uint32_t wordsPerFrame = v.espTdm4 ? 4 : 2;
        Capture c = analyse(buf, total, wordsPerFrame);

        rp(rpt,
           "%-20s i2c=%6u write02=%02X>%02X write07=%02X>%02X sdp=%02X/%02X | "
           "stride=%u live=%u/%u zerorun=%u peak=%d bytes=%u%s",
           v.name, static_cast<unsigned>(v.i2cHz), v.reg02, got02, v.reg07, got07, got11, got12,
           static_cast<unsigned>(c.stride), static_cast<unsigned>(c.liveFrames),
           static_cast<unsigned>(c.frames), static_cast<unsigned>(c.maxZeroRun), c.peak,
           static_cast<unsigned>(total), cok ? "" : " (SOME I2C WRITES FAILED)");

        // Prefer the smallest stride; break ties on the louder capture, since
        // a stride of 1 with a dead signal would be a false positive.
        if (c.stride >= 1 && c.liveFrames > 8 &&
            (c.stride < bestStride || (c.stride == bestStride && c.peak > bestPeak))) {
            bestStride = c.stride;
            bestPeak = c.peak;
            bestIdx = static_cast<int>(vi);
        }

        if (total > 0) {
            char binPath[128];
            snprintf(binPath, sizeof(binPath), "%s/D2_%u.BIN", dir.c_str(),
                     static_cast<unsigned>(vi + 1));
            File bf = SD_MMC.open(binPath, FILE_WRITE);
            if (bf) {
                bf.write(buf, total);
                bf.close();
            }
        }
    }

    i2s_driver_uninstall(kPort);

    // ── Test 3: record with whatever won ───────────────────────────────────
    // If a variant reached stride 1 the codec is finally producing a sample
    // per frame and there is nothing left to diagnose — record real audio
    // with it so it can be played back immediately. If nothing reached 1 the
    // best available configuration still gets recorded, because a recording
    // that is 16x too slow is far more diagnostic to listen to than silence.
    rp(rpt, "");
    if (bestIdx < 0) {
        rp(rpt, "no variant produced usable data — no recording written");
    } else {
        const Variant& v = kVariants[bestIdx];
        rp(rpt, "best variant: %s (stride %u, peak %d)", v.name,
           static_cast<unsigned>(bestStride), bestPeak);
        if (bestStride == 1) {
            rp(rpt, "stride 1 reached — codec clocking correctly, recording 4 s of real audio");
        } else {
            rp(rpt, "stride still %u — recording anyway, it will sound %ux too slow",
               static_cast<unsigned>(bestStride), static_cast<unsigned>(bestStride));
        }

        Wire1.setClock(v.i2cHz);
        delay(5);
        configureEs7210(v.reg02, v.reg07, v.micMask, v.sdp12);
        if (installDiagI2s(v.espTdm4) && buf) {
            delay(200);
            {
                size_t junk = 0;
                for (int i = 0; i < 4; i++)
                    i2s_read(kPort, buf, kCaptureBytes, &junk, pdMS_TO_TICKS(100));
            }

            File wf = SD_MMC.open(currentFilePath_, FILE_WRITE);
            if (wf) {
                WavHeader hdr;
                hdr.sampleRate = kRate;
                hdr.numChannels = 1;
                hdr.bitsPerSample = 16;
                hdr.byteRate = kRate * 2;
                hdr.blockAlign = 2;
                hdr.dataSize = 0;
                wf.write(reinterpret_cast<const uint8_t*>(&hdr), sizeof(hdr));

                const uint32_t wordsPerFrame = v.espTdm4 ? 4 : 2;
                const uint32_t wantFrames = kRate * 4;
                uint32_t wroteFrames = 0;
                int16_t mono[512];
                uint32_t stop = millis() + 8000;

                while (wroteFrames < wantFrames && millis() < stop) {
                    size_t got = 0;
                    if (i2s_read(kPort, buf, kCaptureBytes, &got, pdMS_TO_TICKS(300)) != ESP_OK)
                        break;
                    const int16_t* w = reinterpret_cast<const int16_t*>(buf);
                    const uint32_t frames = (got / 2) / wordsPerFrame;
                    uint32_t idx = 0;
                    for (uint32_t f = 0; f < frames; f++) {
                        // Slot 0 is MIC1 — round 1 showed MIC1 and MIC3 land
                        // in slots 0 and 2 with slots 1 and 3 silent.
                        mono[idx++] = w[f * wordsPerFrame];
                        if (idx == 512) {
                            wf.write(reinterpret_cast<const uint8_t*>(mono), sizeof(mono));
                            wroteFrames += idx;
                            idx = 0;
                        }
                    }
                    if (idx) {
                        wf.write(reinterpret_cast<const uint8_t*>(mono), idx * 2);
                        wroteFrames += idx;
                    }
                }

                const uint32_t dataBytes = wroteFrames * 2;
                wf.seek(0);
                hdr.dataSize = dataBytes;
                hdr.fileSize = sizeof(WavHeader) - 8 + dataBytes;
                wf.write(reinterpret_cast<const uint8_t*>(&hdr), sizeof(hdr));
                wf.close();
                rp(rpt, "recorded %u frames (%u bytes) to %s", static_cast<unsigned>(wroteFrames),
                   static_cast<unsigned>(dataBytes), currentFilePath_.c_str());
            } else {
                rp(rpt, "could not open %s for writing", currentFilePath_.c_str());
            }
        }
        i2s_driver_uninstall(kPort);
    }

    free(buf);
    Wire1.setClock(kNormalI2cHz);

    rp(rpt, "");
    rp(rpt, "=== end of sweep ===");

    File rf = SD_MMC.open(dir + "/DIAG2.TXT", FILE_WRITE);
    if (rf) {
        rf.print(rpt);
        rf.close();
        ESP_LOGI(DTAG, "diagnostic report written (%u bytes)", rpt.length());
    } else {
        ESP_LOGE(DTAG, "cannot write diagnostic report");
    }

    recording_ = false;
    recordTask_ = nullptr;
}

// Same open/start sequence the normal path uses, with the clock registers,
// mic selection and framing left as parameters.
bool AudioRecorder::configureEs7210(uint8_t reg02, uint8_t reg07, uint8_t micMask,
                                    uint8_t sdp12) {
    bool ok = true;
    auto w = [&](uint8_t reg, uint8_t val) {
        if (ok && !writeEs7210Register(reg, val)) ok = false;
    };
    auto upd = [&](uint8_t reg, uint8_t mask, uint8_t data) {
        if (ok && !updateEs7210RegisterBits(reg, mask, data)) ok = false;
    };

    w(kReset00, 0xFF);
    w(kReset00, 0x41);
    w(kClockOff01, 0x3F);
    w(kTime09, 0x30);
    w(kTime0A, 0x30);
    w(kHpf23, 0x2A);
    w(kHpf22, 0x0A);
    w(kHpf20, 0x0A);
    w(kHpf21, 0x2A);
    upd(kModeCfg08, 0x01, 0x00);  // slave — the ESP32 drives BCLK/WS/MCLK
    w(kAnalog40, 0x43);
    w(kBias41, 0x70);
    w(kBias42, 0x70);
    w(kOsr07, reg07);
    w(kMainClk02, reg02);

    for (uint8_t r = kGain43; r <= kGain43 + 3; r++) upd(r, 0x10, 0x00);
    w(kMic12Power4B, 0xFF);
    w(kMic34Power4C, 0xFF);
    if (micMask & 0b0011) {
        upd(kClockOff01, 0x0B, 0x00);
        w(kMic12Power4B, 0x00);
    }
    if (micMask & 0b1100) {
        upd(kClockOff01, 0x15, 0x00);
        w(kMic34Power4C, 0x00);
    }
    for (uint8_t m = 0; m < 4; m++) {
        if (micMask & (1u << m)) upd(kGain43 + m, 0x1F, kGain345db);
    }
    w(kSdp2_12, sdp12);

    // 16-bit Philips on the codec side — the only format round 1 produced
    // sample values that looked like audio in.
    {
        uint8_t reg = 0;
        if (ok && readEs7210Register(kSdp1_11, reg)) {
            w(kSdp1_11, static_cast<uint8_t>(reg & 0xFC));
            if (ok && readEs7210Register(kSdp1_11, reg)) {
                w(kSdp1_11, static_cast<uint8_t>((reg & 0x1F) | 0x60));
            }
        } else {
            ok = false;
        }
    }

    {
        uint8_t clockOff = 0;
        if (ok && readEs7210Register(kClockOff01, clockOff)) w(kClockOff01, clockOff);
        w(kPowerDown06, 0x00);
        w(kAnalog40, 0x43);
        for (uint8_t m = 0; m < 4; m++) w(kPower47 + m, 0x08);
        w(kSdp2_12, sdp12);
        w(kAnalog40, 0x43);
        w(kReset00, 0x71);
        w(kReset00, 0x41);
    }

    // The clock registers again, last, after the two RESET_REG00 writes that
    // end es7210_start() — if anything in the start sequence clears them,
    // this is where it shows up.
    w(kOsr07, reg07);
    w(kMainClk02, reg02);
    return ok;
}

bool AudioRecorder::installDiagI2s(bool tdm4) {
    i2s_config_t cfg = {};
    cfg.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
    cfg.sample_rate = kRate;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format = tdm4 ? I2S_CHANNEL_FMT_MULTIPLE : I2S_CHANNEL_FMT_RIGHT_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags = 0;
    cfg.dma_buf_count = 8;
    cfg.dma_buf_len = 256;
    cfg.use_apll = false;
    cfg.tx_desc_auto_clear = false;
    cfg.fixed_mclk = 0;
    cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    if (tdm4) {
        cfg.bits_per_chan = I2S_BITS_PER_CHAN_32BIT;
        cfg.chan_mask = static_cast<i2s_channel_t>(I2S_TDM_ACTIVE_CH0 | I2S_TDM_ACTIVE_CH1 |
                                                   I2S_TDM_ACTIVE_CH2 | I2S_TDM_ACTIVE_CH3);
        cfg.total_chan = 4;
        cfg.skip_msk = true;
    }

    if (i2s_driver_install(kPort, &cfg, 0, nullptr) != ESP_OK) return false;

    i2s_pin_config_t pins = {};
    pins.mck_io_num = BoardConfig::PIN_AUDIO_MCLK;
    pins.bck_io_num = BoardConfig::PIN_AUDIO_BCLK;
    pins.ws_io_num = BoardConfig::PIN_AUDIO_WS;
    pins.data_out_num = I2S_PIN_NO_CHANGE;
    pins.data_in_num = BoardConfig::PIN_AUDIO_DIN;
    if (i2s_set_pin(kPort, &pins) != ESP_OK) {
        i2s_driver_uninstall(kPort);
        return false;
    }

    if (tdm4) {
        i2s_set_clk(kPort, kRate, 16 | (32 << 16),
                    static_cast<i2s_channel_t>(I2S_TDM_ACTIVE_CH0 | I2S_TDM_ACTIVE_CH1 |
                                               I2S_TDM_ACTIVE_CH2 | I2S_TDM_ACTIVE_CH3));
    } else {
        i2s_set_clk(kPort, kRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
    }
    return true;
}

#endif  // AUDIO_DIAG
