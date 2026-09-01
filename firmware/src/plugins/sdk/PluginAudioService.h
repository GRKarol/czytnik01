// firmware/src/plugins/sdk/PluginAudioService.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PluginAudioService {
    bool (*beep)(void);
    bool (*available)(void);

    // ─── Recording (microphone → SD card as WAV) ────────────────────────
    /// Start recording audio to a file (path relative to plugin sandbox).
    /// Returns true if recording started successfully.
    bool (*startRecording)(const char* relativePath);

    /// Stop the current recording and finalize the WAV file.
    /// Returns true if recording was stopped and file saved correctly.
    bool (*stopRecording)(void);

    /// Returns true if currently recording.
    bool (*isRecording)(void);

    /// Get elapsed recording time in milliseconds.
    uint32_t (*recordingElapsedMs)(void);

    // ─── Playback (WAV file → speaker) ──────────────────────────────────
    /// Start playing a WAV file (path relative to plugin sandbox).
    bool (*startPlayback)(const char* relativePath);

    /// Stop current playback.
    bool (*stopPlayback)(void);

    /// Returns true if currently playing audio.
    bool (*isPlaying)(void);

    /// Get elapsed playback time in milliseconds.
    uint32_t (*playbackElapsedMs)(void);

    /// Get total duration of currently playing file in milliseconds.
    uint32_t (*playbackTotalMs)(void);

    /// Pause/resume the current playback without tearing down the codec
    /// session — resume is instant, no re-init pop. No-ops if not playing.
    void (*pausePlayback)(void);
    void (*resumePlayback)(void);
    bool (*isPaused)(void);

    /// Jump playback by deltaMs (negative = backward), clamped to
    /// [0, playbackTotalMs()]. No-op if not playing.
    void (*seekPlaybackBy)(int32_t deltaMs);

    // ─── Volume (persisted, shared by every playback path) ──────────────
    /// Get current playback volume, 0-100.
    uint8_t (*getVolume)(void);

    /// Set playback volume, 0-100 (values above 100 are clamped). Takes
    /// effect immediately if audio is currently playing.
    void (*setVolume)(uint8_t percent);
} PluginAudioService;

#ifdef __cplusplus
}
#endif
