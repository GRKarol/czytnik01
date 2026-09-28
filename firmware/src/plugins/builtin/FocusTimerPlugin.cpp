// firmware/src/plugins/builtin/FocusTimerPlugin.cpp
#include "plugins/builtin/FocusTimerPlugin.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace focustimer {

// ─── Session (ported from rsvpnano's focus::Session) ────────────────────────

void Session::begin(uint16_t focusMinutes, uint16_t breakMinutes, uint8_t rounds) {
    focusDurationMs_ = static_cast<uint32_t>(focusMinutes) * 60UL * 1000UL;
    breakDurationMs_ = static_cast<uint32_t>(breakMinutes) * 60UL * 1000UL;
    rounds_ = rounds;
    round_ = 1;
    phase_ = Phase::WaitingFocus;
    activeSide_ = Orientation::Unknown;
    waitTarget_ = Orientation::Unknown;
    startedMs_ = 0;
    durationMs_ = 0;
    pausedRemainingMs_ = 0;
    targetPresentAtWaitStart_ = false;
    completionCuePending_ = false;
    manual_ = false;
}

void Session::update(uint32_t nowMs, Orientation orientation) {
    if ((phase_ == Phase::Focus || phase_ == Phase::Break) && nowMs - startedMs_ >= durationMs_) {
        finishPhase(orientation);
        completionCuePending_ = true;
        return;
    }

    switch (phase_) {
        case Phase::WaitingFocus:
        case Phase::WaitingBreak:
            if (!shortSide(orientation) || orientation != waitTarget_) {
                targetPresentAtWaitStart_ = false;
            }
            if (shortSide(orientation) &&
                (waitTarget_ == Orientation::Unknown || orientation == waitTarget_) &&
                !targetPresentAtWaitStart_) {
                startPhase(phase_ == Phase::WaitingFocus ? Phase::Focus : Phase::Break, nowMs, orientation, false);
            }
            break;
        case Phase::Focus:
        case Phase::Break:
            if (!manual_ && orientation == Orientation::Flat) {
                pausedRemainingMs_ = remainingMs(nowMs);
                phase_ = phase_ == Phase::Focus ? Phase::PausedFocus : Phase::PausedBreak;
            }
            break;
        case Phase::PausedFocus:
        case Phase::PausedBreak:
            if (!manual_ && orientation == activeSide_) {
                durationMs_ = pausedRemainingMs_;
                startedMs_ = nowMs;
                phase_ = phase_ == Phase::PausedFocus ? Phase::Focus : Phase::Break;
            }
            break;
        case Phase::Complete:
            break;
    }
}

void Session::tap(uint32_t nowMs) {
    switch (phase_) {
        case Phase::WaitingFocus:
        case Phase::WaitingBreak:
            startPhase(phase_ == Phase::WaitingFocus ? Phase::Focus : Phase::Break, nowMs, Orientation::Unknown,
                       true);
            break;
        case Phase::Focus:
        case Phase::Break:
            pausedRemainingMs_ = remainingMs(nowMs);
            phase_ = phase_ == Phase::Focus ? Phase::PausedFocus : Phase::PausedBreak;
            manual_ = true;
            break;
        case Phase::PausedFocus:
        case Phase::PausedBreak:
            durationMs_ = pausedRemainingMs_;
            startedMs_ = nowMs;
            phase_ = phase_ == Phase::PausedFocus ? Phase::Focus : Phase::Break;
            manual_ = true;
            break;
        case Phase::Complete:
            break;
    }
}

void Session::stop() {
    phase_ = Phase::Complete;
    completionCuePending_ = false;
}

uint32_t Session::remainingMs(uint32_t nowMs) const {
    if (phase_ == Phase::PausedFocus || phase_ == Phase::PausedBreak) return pausedRemainingMs_;
    if (phase_ != Phase::Focus && phase_ != Phase::Break) return 0;
    const uint32_t elapsed = nowMs - startedMs_;
    return elapsed >= durationMs_ ? 0 : durationMs_ - elapsed;
}

uint16_t Session::progressPermille(uint32_t nowMs) const {
    if (phase_ == Phase::WaitingBreak) return 1000;
    if (phase_ == Phase::WaitingFocus) return 0;
    const uint32_t total = phase_ == Phase::Focus || phase_ == Phase::PausedFocus     ? focusDurationMs_
                            : phase_ == Phase::Break || phase_ == Phase::PausedBreak  ? breakDurationMs_
                                                                                       : 0;
    if (total == 0) return phase_ == Phase::Complete ? 1000 : 0;
    const uint32_t remaining = remainingMs(nowMs);
    const uint32_t capped = remaining < total ? remaining : total;
    return static_cast<uint16_t>(static_cast<uint64_t>(total - capped) * 1000ULL / total);
}

bool Session::consumeCompletionCue() {
    const bool pending = completionCuePending_;
    completionCuePending_ = false;
    return pending;
}

bool Session::shortSide(Orientation orientation) {
    return orientation == Orientation::ShortA || orientation == Orientation::ShortB;
}

Orientation Session::opposite(Orientation orientation) {
    return orientation == Orientation::ShortA   ? Orientation::ShortB
           : orientation == Orientation::ShortB ? Orientation::ShortA
                                                 : Orientation::Unknown;
}

void Session::startPhase(Phase phase, uint32_t nowMs, Orientation orientation, bool manual) {
    phase_ = phase;
    manual_ = manual;
    activeSide_ = orientation;
    startedMs_ = nowMs;
    durationMs_ = phase == Phase::Focus ? focusDurationMs_ : breakDurationMs_;
    pausedRemainingMs_ = 0;
    waitTarget_ = Orientation::Unknown;
    targetPresentAtWaitStart_ = false;
}

void Session::finishPhase(Orientation orientation) {
    manual_ = false;
    if (phase_ == Phase::Focus && round_ >= rounds_) {
        phase_ = Phase::Complete;
        durationMs_ = 0;
        return;
    }
    if (phase_ == Phase::Break) ++round_;
    phase_ = phase_ == Phase::Focus ? Phase::WaitingBreak : Phase::WaitingFocus;
    waitTarget_ = opposite(activeSide_);
    targetPresentAtWaitStart_ = orientation == waitTarget_;
    durationMs_ = 0;
    pausedRemainingMs_ = 0;
}

// ─── OrientationSampler (ported from rsvpnano's focus::OrientationReader) ───

namespace {
constexpr uint32_t kSampleIntervalMs = 50;
constexpr uint32_t kStableMs = 700;
constexpr float kSideThreshold = 0.78f;
constexpr float kCrossLimit = 0.42f;
constexpr float kFlatThreshold = 0.84f;
}  // namespace

bool OrientationSampler::probe(uint32_t nowMs) {
    if (available_) return true;
    if (!imu_ || !imu_->available) return false;
    if (probed_ && nowMs - lastProbeMs_ < 2000) return false;
    probed_ = true;
    lastProbeMs_ = nowMs;
    available_ = imu_->available();
    return available_;
}

Orientation OrientationSampler::update(uint32_t nowMs) {
    if (!probe(nowMs)) return Orientation::Unknown;
    if (nowMs - lastSampleMs_ < kSampleIntervalMs) return stable_;
    lastSampleMs_ = nowMs;

    float x = 0;
    float y = 0;
    float z = 0;
    if (!imu_->readAccelerometer(&x, &y, &z)) return stable_;

    const Orientation measured = classify(x, y, z);
    if (measured != candidate_) {
        candidate_ = measured;
        candidateSinceMs_ = nowMs;
    } else if (nowMs - candidateSinceMs_ >= kStableMs) {
        stable_ = candidate_;
    }
    return stable_;
}

Orientation OrientationSampler::classify(float x, float y, float z) {
    if (fabsf(z) >= kFlatThreshold && fabsf(x) <= 0.30f && fabsf(y) <= 0.30f) return Orientation::Flat;
    if (fabsf(y) >= kSideThreshold && fabsf(x) <= kCrossLimit && fabsf(z) <= kCrossLimit) return Orientation::Flat;
    if (x >= kSideThreshold && fabsf(y) <= kCrossLimit && fabsf(z) <= kCrossLimit) return Orientation::ShortA;
    if (x <= -kSideThreshold && fabsf(y) <= kCrossLimit && fabsf(z) <= kCrossLimit) return Orientation::ShortB;
    return Orientation::Unknown;
}

}  // namespace focustimer

// ─── FocusTimerCore ──────────────────────────────────────────────────────────

namespace {

using focustimer::Orientation;
using focustimer::Phase;
using focustimer::Preset;

constexpr Preset kPresets[focustimer::kPresetCount] = {
    {25, 5, 4},  // Pomodoro
    {15, 5, 3},  // short session
    {50, 10, 2}, // deep work
};

// ─── Localization ────────────────────────────────────────────────────────────
//
// Same scheme as DictaphonePlugin's DictStr: the strings live in
// tools/translations.csv (FtStr.* rows, Polish with diacritics) and come
// back through PluginDisplayService::pluginTr(). The bridge tells the two
// plugins' tables apart by the key's high byte (kFtStrTable).
enum class FtStr : uint8_t {
    PresetPomodoro,
    PresetShort,
    PresetDeep,
    Start,
    ModeReadyFocus,
    ModeFocus,
    ModePaused,
    ModeReadyBreak,
    ModeBreak,
    ModeComplete,
    InstrFlipToStartFocus,
    InstrFlipToStartBreak,
    InstrLayFlatToPause,
    InstrFlipToResume,
    InstrTapToPause,
    InstrTapToResume,
    InstrTapToFinish,
    Round,
};

constexpr uint16_t kFtStrTable = 0x0100;

PluginDisplayService* s_display = nullptr;

const char* ftText(FtStr key, int lang) {
    if (!s_display || !s_display->pluginTr) return "";
    return s_display->pluginTr(static_cast<uint16_t>(kFtStrTable | static_cast<uint16_t>(key)), lang);
}

focustimer::FocusTimerCore* s_instance = nullptr;

}  // namespace

namespace focustimer {

FocusTimerCore::FocusTimerCore(PluginDisplayService* display, PluginAudioService* audio,
                               PluginImuService* imu, PluginStorageService* storage)
    : display_(display), audio_(audio), imu_(imu), storage_(storage) {}

bool FocusTimerCore::begin() {
    s_display = display_;
    orientation_.begin(imu_);
    loadPreset();
    screen_ = Screen::Main;
    return true;
}

void FocusTimerCore::shutdown() {
    session_.stop();
}

void FocusTimerCore::goToScreen(Screen screen) {
    screen_ = screen;
}

void FocusTimerCore::startSession() {
    const Preset& preset = kPresets[presetIndex_];
    session_.begin(preset.focusMinutes, preset.breakMinutes, preset.rounds);
}

void FocusTimerCore::loadPreset() {
    if (!storage_ || !storage_->loadInt) return;
    int32_t value = 0;
    storage_->loadInt("config.txt", "preset", &value, 0, kPresetCount - 1, 0);
    presetIndex_ = static_cast<uint8_t>(value);
}

void FocusTimerCore::savePreset() {
    if (!storage_ || !storage_->saveInt) return;
    storage_->saveInt("config.txt", "preset", presetIndex_);
}

void FocusTimerCore::update(uint32_t nowMs) {
    lastNowMs_ = nowMs;
    const Orientation orientation = orientation_.update(nowMs);

    if (screen_ != Screen::Session) return;

    session_.update(nowMs, orientation);
    if (session_.consumeCompletionCue() && audio_ && audio_->beep) {
        audio_->beep();
    }
}

void FocusTimerCore::handleButton(const PluginButtonEvent* event) {
    if (!event || !event->pressed) return;
    // Only the boot button (id 0) ever reaches a plugin — see
    // DictaphonePlugin::handleButton for why (power is consumed globally
    // as "exit plugin" before it gets here).
    if (event->buttonId != 0) return;

    switch (screen_) {
        case Screen::Main:
            presetIndex_ = static_cast<uint8_t>((presetIndex_ + 1) % kPresetCount);
            savePreset();
            break;
        case Screen::Session:
            session_.stop();
            goToScreen(Screen::Main);
            break;
    }
}

void FocusTimerCore::handleTouch(const PluginTouchEvent* event) {
    if (!event) return;
    // Only handle touch end (tap) — the session screen is orientation
    // driven, not touch driven, so there is nothing to track mid-drag here.
    if (event->phase != 2) return;
    if (event->timestampMs - lastActionMs_ < kActionCooldownMs) return;
    lastActionMs_ = event->timestampMs;

    switch (screen_) {
        case Screen::Main: {
            const int width = display_ && display_->logicalWidth ? display_->logicalWidth() : 640;
            if (event->x < static_cast<uint16_t>(width / 2)) {
                presetIndex_ = static_cast<uint8_t>((presetIndex_ + 1) % kPresetCount);
                savePreset();
            } else {
                startSession();
                goToScreen(Screen::Session);
            }
            break;
        }
        case Screen::Session:
            if (event->x < kBackZoneW && event->y < kBackZoneH) {
                session_.stop();
                goToScreen(Screen::Main);
                break;
            }
            // Once it is over, any tap dismisses the summary and heads
            // back to the picker. Before that a tap starts, pauses or
            // resumes the phase (flipping the device still works too).
            if (session_.phase() == Phase::Complete) {
                goToScreen(Screen::Main);
            } else {
                session_.tap(event->timestampMs);
            }
            break;
    }
}

void FocusTimerCore::draw() {
    switch (screen_) {
        case Screen::Main:
            drawMain();
            break;
        case Screen::Session:
            drawSession(lastNowMs_);
            break;
    }
}

void FocusTimerCore::drawMain() {
    if (!display_ || !display_->renderButtonPair) return;
    const int lang = display_->languageIndex ? display_->languageIndex() : 0;
    const Preset& preset = kPresets[presetIndex_];
    const FtStr nameKey = presetIndex_ == 0   ? FtStr::PresetPomodoro
                           : presetIndex_ == 1 ? FtStr::PresetShort
                                                : FtStr::PresetDeep;

    char leftLabel[40];
    snprintf(leftLabel, sizeof(leftLabel), "%s %u/%u x%u", ftText(nameKey, lang), preset.focusMinutes,
             preset.breakMinutes, preset.rounds);

    display_->renderButtonPair(leftLabel, PLUGIN_ICON_NONE, false, ftText(FtStr::Start, lang), PLUGIN_ICON_PLAY);
}

void FocusTimerCore::drawSession(uint32_t nowMs) {
    if (!display_ || !display_->renderFocusTimerScreen) return;
    const int lang = display_->languageIndex ? display_->languageIndex() : 0;
    const Preset& preset = kPresets[presetIndex_];
    const Phase phase = session_.phase();

    const bool breakPhase =
        phase == Phase::WaitingBreak || phase == Phase::Break || phase == Phase::PausedBreak;

    FtStr modeKey;
    FtStr instrKey = FtStr::InstrFlipToStartFocus;
    bool hasInstruction = true;
    int progressPercent = -1;

    switch (phase) {
        case Phase::WaitingFocus:
            modeKey = FtStr::ModeReadyFocus;
            instrKey = FtStr::InstrFlipToStartFocus;
            progressPercent = -1;
            break;
        case Phase::Focus:
            modeKey = FtStr::ModeFocus;
            instrKey = session_.manual() ? FtStr::InstrTapToPause : FtStr::InstrLayFlatToPause;
            progressPercent = session_.progressPermille(nowMs) / 10;
            break;
        case Phase::PausedFocus:
            modeKey = FtStr::ModePaused;
            instrKey = session_.manual() ? FtStr::InstrTapToResume : FtStr::InstrFlipToResume;
            progressPercent = session_.progressPermille(nowMs) / 10;
            break;
        case Phase::WaitingBreak:
            modeKey = FtStr::ModeReadyBreak;
            instrKey = FtStr::InstrFlipToStartBreak;
            progressPercent = -1;
            break;
        case Phase::Break:
            modeKey = FtStr::ModeBreak;
            instrKey = session_.manual() ? FtStr::InstrTapToPause : FtStr::InstrLayFlatToPause;
            progressPercent = session_.progressPermille(nowMs) / 10;
            break;
        case Phase::PausedBreak:
            modeKey = FtStr::ModePaused;
            instrKey = session_.manual() ? FtStr::InstrTapToResume : FtStr::InstrFlipToResume;
            progressPercent = session_.progressPermille(nowMs) / 10;
            break;
        case Phase::Complete:
            modeKey = FtStr::ModeComplete;
            instrKey = FtStr::InstrTapToFinish;
            progressPercent = 100;
            break;
    }
    (void)hasInstruction;

    uint32_t remaining = session_.remainingMs(nowMs);
    if (phase == Phase::WaitingFocus) {
        remaining = static_cast<uint32_t>(preset.focusMinutes) * 60UL * 1000UL;
    } else if (phase == Phase::WaitingBreak) {
        remaining = static_cast<uint32_t>(preset.breakMinutes) * 60UL * 1000UL;
    }
    const uint32_t seconds = (remaining + 999UL) / 1000UL;

    char timeText[8];
    snprintf(timeText, sizeof(timeText), "%02u:%02u", static_cast<unsigned>(seconds / 60UL),
             static_cast<unsigned>(seconds % 60UL));

    char instruction[96];
    if (phase == Phase::Complete) {
        snprintf(instruction, sizeof(instruction), "%s", ftText(instrKey, lang));
    } else if (preset.rounds > 1) {
        snprintf(instruction, sizeof(instruction), "%s %u/%u. %s", ftText(FtStr::Round, lang), session_.round(),
                 session_.rounds(), ftText(instrKey, lang));
    } else {
        snprintf(instruction, sizeof(instruction), "%s", ftText(instrKey, lang));
    }

    display_->renderFocusTimerScreen(ftText(modeKey, lang), "", timeText, instruction, "", progressPercent,
                                     breakPhase);
}

}  // namespace focustimer

// ─── Plugin SDK VTable glue ──────────────────────────────────────────────────

namespace {

PluginResult focusTimerInit(PluginContext* ctx) {
    s_instance = new focustimer::FocusTimerCore(ctx->display, ctx->audio, ctx->imu, ctx->storage);
    if (!s_instance) return PLUGIN_ERROR_MEMORY;
    if (!s_instance->begin()) {
        delete s_instance;
        s_instance = nullptr;
        return PLUGIN_ERROR_INIT;
    }
    return PLUGIN_OK;
}

void focusTimerDestroy() {
    if (s_instance) {
        s_instance->shutdown();
        delete s_instance;
        s_instance = nullptr;
    }
    s_display = nullptr;
}

void focusTimerUpdate(uint32_t nowMs) {
    if (s_instance) s_instance->update(nowMs);
}

void focusTimerHandleButton(const PluginButtonEvent* event) {
    if (s_instance) s_instance->handleButton(event);
}

void focusTimerHandleTouch(const PluginTouchEvent* event) {
    if (s_instance) s_instance->handleTouch(event);
}

void focusTimerDraw() {
    if (s_instance) s_instance->draw();
}

PluginInfo focusTimerGetInfo() {
    return {"Klepsydra", "1.0.0", PLUGIN_SDK_VERSION};
}

}  // namespace

PluginVTable FocusTimerPlugin::vtable() {
    return {
        focusTimerInit, focusTimerDestroy,     focusTimerUpdate, focusTimerHandleButton,
        focusTimerHandleTouch, focusTimerDraw, focusTimerGetInfo,
    };
}
