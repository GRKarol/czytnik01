// firmware/src/plugins/builtin/FocusTimerPlugin.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "plugins/sdk/PluginSdk.h"
#include "plugins/sdk/PluginDisplayService.h"
#include "plugins/sdk/PluginAudioService.h"
#include "plugins/sdk/PluginImuService.h"
#include "plugins/sdk/PluginStorageService.h"

namespace focustimer {

// Which short edge (or flat-on-table) the device is currently resting on.
// Flipping between ShortA/ShortB drives the session state machine; Flat
// pauses it. Ported from rsvpnano's focus::Orientation.
enum class Orientation : uint8_t {
    Unknown,
    ShortA,
    ShortB,
    Flat,
};

// Ported from rsvpnano's focus::Phase — a focus/break round is always in
// exactly one of these phases.
enum class Phase : uint8_t {
    WaitingFocus,
    Focus,
    PausedFocus,
    WaitingBreak,
    Break,
    PausedBreak,
    Complete,
};

// Countdown state machine for one focus/break session. Ported near-verbatim
// from rsvpnano's focus::Session (src/focus/FocusSession.{h,cpp} in
// https://github.com/ionutdecebal/rsvpnano) — same orientation-flip rules,
// stripped of its TOML-backed multi-timer settings model.
class Session {
 public:
    void begin(uint16_t focusMinutes, uint16_t breakMinutes, uint8_t rounds);
    void update(uint32_t nowMs, Orientation orientation);
    void stop();

    Phase phase() const { return phase_; }
    uint8_t round() const { return round_; }
    uint8_t rounds() const { return rounds_; }
    uint32_t remainingMs(uint32_t nowMs) const;
    uint16_t progressPermille(uint32_t nowMs) const;
    bool consumeCompletionCue();

 private:
    static bool shortSide(Orientation orientation);
    static Orientation opposite(Orientation orientation);
    void startPhase(Phase phase, uint32_t nowMs, Orientation orientation);
    void finishPhase(Orientation orientation);

    Phase phase_ = Phase::Complete;
    Orientation activeSide_ = Orientation::Unknown;
    Orientation waitTarget_ = Orientation::Unknown;
    uint32_t startedMs_ = 0;
    uint32_t durationMs_ = 0;
    uint32_t pausedRemainingMs_ = 0;
    uint32_t focusDurationMs_ = 0;
    uint32_t breakDurationMs_ = 0;
    uint8_t round_ = 0;
    uint8_t rounds_ = 0;
    bool targetPresentAtWaitStart_ = false;
    bool completionCuePending_ = false;
};

// Classifies the device's resting orientation from the accelerometer, with
// a debounce window so a flip only registers once it has settled. Ported
// from rsvpnano's focus::OrientationReader, adapted to sample through the
// plugin SDK's PluginImuService instead of talking to the QMI8658 directly
// (the host firmware already owns that I2C bus).
class OrientationSampler {
 public:
    void begin(PluginImuService* imu) { imu_ = imu; }
    Orientation update(uint32_t nowMs);
    bool available() const { return imu_ && imu_->available && imu_->available(); }

 private:
    static Orientation classify(float x, float y, float z);

    PluginImuService* imu_ = nullptr;
    Orientation candidate_ = Orientation::Unknown;
    Orientation stable_ = Orientation::Unknown;
    uint32_t candidateSinceMs_ = 0;
    uint32_t lastSampleMs_ = 0;
};

struct Preset {
    uint16_t focusMinutes;
    uint16_t breakMinutes;
    uint8_t rounds;
};

// Small fixed set of named presets, standing in for rsvpnano's editable
// TOML-backed timer library — same idea (pick a focus/break/rounds combo),
// trimmed to what fits this device's compact touch UI.
static constexpr uint8_t kPresetCount = 3;

class FocusTimerCore {
 public:
    enum class Screen : uint8_t {
        Main,     // Preset picker + start button
        Session,  // Running/paused/waiting/complete countdown
    };

    FocusTimerCore(PluginDisplayService* display, PluginAudioService* audio,
                   PluginImuService* imu, PluginStorageService* storage);

    bool begin();
    void update(uint32_t nowMs);
    void handleButton(const PluginButtonEvent* event);
    void handleTouch(const PluginTouchEvent* event);
    void draw();

    // Stops any in-flight session so it doesn't keep counting down after
    // the plugin is unloaded (the power button's global "exit plugin"
    // doesn't know or care what screen we're on — see DictaphonePlugin's
    // shutdown() for the same reasoning with a live AudioRecorder).
    void shutdown();

 private:
    void goToScreen(Screen screen);
    void startSession();
    void loadPreset();
    void savePreset();

    void drawMain();
    void drawSession(uint32_t nowMs);

    PluginDisplayService* display_;
    PluginAudioService* audio_;
    PluginImuService* imu_;
    PluginStorageService* storage_;

    Screen screen_ = Screen::Main;
    uint8_t presetIndex_ = 0;
    Session session_;
    OrientationSampler orientation_;
    uint32_t lastNowMs_ = 0;

    // Debounces the touch controller's contact-bounce phantom second
    // release, same as DictaphoneCore::lastActionMs_.
    uint32_t lastActionMs_ = 0;
    static constexpr uint32_t kActionCooldownMs = 350;
};

}  // namespace focustimer

/// Plugin SDK vtable entry points for the Focus Timer ("Klepsydra") built-in
/// plugin.
namespace FocusTimerPlugin {
PluginVTable vtable();
}
