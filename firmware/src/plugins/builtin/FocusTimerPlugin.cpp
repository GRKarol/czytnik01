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
                startPhase(phase_ == Phase::WaitingFocus ? Phase::Focus : Phase::Break, nowMs, orientation);
            }
            break;
        case Phase::Focus:
        case Phase::Break:
            if (orientation == Orientation::Flat) {
                pausedRemainingMs_ = remainingMs(nowMs);
                phase_ = phase_ == Phase::Focus ? Phase::PausedFocus : Phase::PausedBreak;
            }
            break;
        case Phase::PausedFocus:
        case Phase::PausedBreak:
            if (orientation == activeSide_) {
                durationMs_ = pausedRemainingMs_;
                startedMs_ = nowMs;
                phase_ = phase_ == Phase::PausedFocus ? Phase::Focus : Phase::Break;
            }
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

void Session::startPhase(Phase phase, uint32_t nowMs, Orientation orientation) {
    phase_ = phase;
    activeSide_ = orientation;
    startedMs_ = nowMs;
    durationMs_ = phase == Phase::Focus ? focusDurationMs_ : breakDurationMs_;
    pausedRemainingMs_ = 0;
    waitTarget_ = Orientation::Unknown;
    targetPresentAtWaitStart_ = false;
}

void Session::finishPhase(Orientation orientation) {
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

Orientation OrientationSampler::update(uint32_t nowMs) {
    if (!available()) return Orientation::Unknown;
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
// Same reasoning as DictaphonePlugin's DictStr table: this plugin is built
// standalone from the app, so it carries its own copy of the app's 6
// language ordering (0=English, 1=Spanish, 2=French, 3=German, 4=Romanian,
// 5=Polish) instead of depending on app/Translations.h.
enum class FtStr : uint8_t {
    PresetPomodoro,
    PresetShort,
    PresetDeep,
    Start,
    ModeReadyFocus,
    ModeFocus,
    ModePausedFocus,
    ModeReadyBreak,
    ModeBreak,
    ModePausedBreak,
    ModeComplete,
    InstrFlipToStartFocus,
    InstrFlipToStartBreak,
    InstrLayFlatToPause,
    InstrFlipToResume,
    InstrTapToFinish,
    Round,
};

const char* ftText(FtStr key, int lang) {
    switch (key) {
        case FtStr::PresetPomodoro:
            switch (lang) {
                case 1: return "Pomodoro";
                case 2: return "Pomodoro";
                case 3: return "Pomodoro";
                case 4: return "Pomodoro";
                case 5: return "Pomodoro";
                default: return "Pomodoro";
            }
        case FtStr::PresetShort:
            switch (lang) {
                case 1: return "Sesion corta";
                case 2: return "Session courte";
                case 3: return "Kurze Sitzung";
                case 4: return "Sesiune scurta";
                case 5: return "Krotka sesja";
                default: return "Short session";
            }
        case FtStr::PresetDeep:
            switch (lang) {
                case 1: return "Trabajo profundo";
                case 2: return "Travail profond";
                case 3: return "Vertiefte Arbeit";
                case 4: return "Munca profunda";
                case 5: return "Dluga sesja";
                default: return "Deep work";
            }
        case FtStr::Start:
            switch (lang) {
                case 1: return "Iniciar";
                case 2: return "Demarrer";
                case 3: return "Start";
                case 4: return "Start";
                case 5: return "Start";
                default: return "Start";
            }
        case FtStr::ModeReadyFocus:
            switch (lang) {
                case 1: return "Listo";
                case 2: return "Pret";
                case 3: return "Bereit";
                case 4: return "Gata";
                case 5: return "Gotowy";
                default: return "Ready";
            }
        case FtStr::ModeFocus:
            switch (lang) {
                case 1: return "Concentracion";
                case 2: return "Concentration";
                case 3: return "Fokus";
                case 4: return "Concentrare";
                case 5: return "Skupienie";
                default: return "Focus";
            }
        case FtStr::ModePausedFocus:
        case FtStr::ModePausedBreak:
            switch (lang) {
                case 1: return "Pausa";
                case 2: return "Pause";
                case 3: return "Pause";
                case 4: return "Pauza";
                case 5: return "Pauza";
                default: return "Paused";
            }
        case FtStr::ModeReadyBreak:
            switch (lang) {
                case 1: return "Descanso pronto";
                case 2: return "Pause bientot";
                case 3: return "Pause bald";
                case 4: return "Pauza in curand";
                case 5: return "Zaraz przerwa";
                default: return "Break soon";
            }
        case FtStr::ModeBreak:
            switch (lang) {
                case 1: return "Descanso";
                case 2: return "Pause";
                case 3: return "Pause";
                case 4: return "Pauza";
                case 5: return "Przerwa";
                default: return "Break";
            }
        case FtStr::ModeComplete:
            switch (lang) {
                case 1: return "Completado";
                case 2: return "Termine";
                case 3: return "Fertig";
                case 4: return "Finalizat";
                case 5: return "Koniec!";
                default: return "Complete";
            }
        case FtStr::InstrFlipToStartFocus:
            switch (lang) {
                case 1: return "Apoya sobre el lado corto para empezar";
                case 2: return "Posez sur la tranche pour commencer";
                case 3: return "Auf die Schmalseite stellen zum Starten";
                case 4: return "Aseaza pe latura scurta pentru start";
                case 5: return "Postaw na krotszym boku, by zaczac";
                default: return "Stand it on a short edge to start";
            }
        case FtStr::InstrFlipToStartBreak:
            switch (lang) {
                case 1: return "Gira al lado opuesto para el descanso";
                case 2: return "Retournez pour la pause";
                case 3: return "Zur anderen Seite drehen fuer die Pause";
                case 4: return "Intoarce pe partea opusa pentru pauza";
                case 5: return "Odwroc na druga strone, by zaczac przerwe";
                default: return "Flip to the other side for the break";
            }
        case FtStr::InstrLayFlatToPause:
            switch (lang) {
                case 1: return "Ponlo plano para pausar";
                case 2: return "Posez a plat pour mettre en pause";
                case 3: return "Flach legen zum Pausieren";
                case 4: return "Aseaza plat pentru pauza";
                case 5: return "Poloz plasko, by zapauzowac";
                default: return "Lay it flat to pause";
            }
        case FtStr::InstrFlipToResume:
            switch (lang) {
                case 1: return "Vuelve a la posicion anterior para continuar";
                case 2: return "Revenez a la position precedente pour reprendre";
                case 3: return "Zurueckdrehen zum Fortsetzen";
                case 4: return "Revino la pozitia anterioara pentru a continua";
                case 5: return "Wroc do poprzedniej pozycji, by wznowic";
                default: return "Flip back to resume";
            }
        case FtStr::InstrTapToFinish:
            switch (lang) {
                case 1: return "Toca para terminar";
                case 2: return "Touchez pour terminer";
                case 3: return "Zum Beenden tippen";
                case 4: return "Atinge pentru a termina";
                case 5: return "Dotknij, by zakonczyc";
                default: return "Tap to finish";
            }
        case FtStr::Round:
            switch (lang) {
                case 1: return "Ronda";
                case 2: return "Tour";
                case 3: return "Runde";
                case 4: return "Runda";
                case 5: return "Runda";
                default: return "Round";
            }
    }
    return "";
}

focustimer::FocusTimerCore* s_instance = nullptr;

}  // namespace

namespace focustimer {

FocusTimerCore::FocusTimerCore(PluginDisplayService* display, PluginAudioService* audio,
                               PluginImuService* imu, PluginStorageService* storage)
    : display_(display), audio_(audio), imu_(imu), storage_(storage) {}

bool FocusTimerCore::begin() {
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
            // Once it is over, any tap dismisses the summary and heads
            // back to the picker. While a round is running, control stays
            // with the accelerometer on purpose — see InstrLayFlatToPause.
            if (session_.phase() == Phase::Complete) {
                goToScreen(Screen::Main);
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
            instrKey = FtStr::InstrLayFlatToPause;
            progressPercent = session_.progressPermille(nowMs) / 10;
            break;
        case Phase::PausedFocus:
            modeKey = FtStr::ModePausedFocus;
            instrKey = FtStr::InstrFlipToResume;
            progressPercent = session_.progressPermille(nowMs) / 10;
            break;
        case Phase::WaitingBreak:
            modeKey = FtStr::ModeReadyBreak;
            instrKey = FtStr::InstrFlipToStartBreak;
            progressPercent = -1;
            break;
        case Phase::Break:
            modeKey = FtStr::ModeBreak;
            instrKey = FtStr::InstrLayFlatToPause;
            progressPercent = session_.progressPermille(nowMs) / 10;
            break;
        case Phase::PausedBreak:
            modeKey = FtStr::ModePausedBreak;
            instrKey = FtStr::InstrFlipToResume;
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
