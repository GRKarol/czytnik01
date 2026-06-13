// firmware/src/plugins/builtin/DictaphonePlugin.cpp
#include "plugins/builtin/DictaphonePlugin.h"

#include <string.h>
#include <stdio.h>

namespace {

// Menu items for library context menu (shown via touch actions)
static constexpr uint8_t kLibraryVisibleRows = 5;

// Singleton instance
DictaphoneCore* s_instance = nullptr;

}  // namespace

// ─── DictaphoneCore Implementation ─────────────────────────────────────────

DictaphoneCore::DictaphoneCore(PluginDisplayService* display,
                               PluginAudioService* audio,
                               PluginStorageService* storage)
    : display_(display), audio_(audio), storage_(storage) {}

bool DictaphoneCore::begin() {
    // Ensure recordings directory exists
    if (storage_ && storage_->mkdir) {
        storage_->mkdir("recordings");
    }

    scanRecordings();
    return true;
}

void DictaphoneCore::update(uint32_t nowMs) {
    lastUpdateMs_ = nowMs;

    // Auto-stop recording at max duration (handled by AudioRecorder but also check here)
    if (screen_ == Screen::Recording && audio_ && audio_->isRecording) {
        if (!audio_->isRecording()) {
            // Recording stopped externally (max duration reached or error)
            // Add the file to our recordings list
            if (currentRecordingName_[0] != '\0' && recordingCount_ < kDictMaxRecordings) {
                strncpy(recordingNames_[recordingCount_], currentRecordingName_, kDictMaxFilenameLen - 1);
                recordingNames_[recordingCount_][kDictMaxFilenameLen - 1] = '\0';
                recordingCount_++;
                saveIndex();
            }
            currentRecordingName_[0] = '\0';
            goToScreen(Screen::Library);
        }
    }

    // Auto-return when playback finishes
    if (screen_ == Screen::Playing && audio_ && audio_->isPlaying) {
        if (!audio_->isPlaying()) {
            goToScreen(Screen::Library);
        }
    }
}

void DictaphoneCore::handleButton(const PluginButtonEvent* event) {
    if (!event || !event->pressed) return;

    switch (screen_) {
        case Screen::Main:
            if (event->buttonId == 0) {
                // Boot button — open library
                goToScreen(Screen::Library);
            } else if (event->buttonId == 1) {
                // Power button — start recording
                startRecording();
            }
            break;

        case Screen::Recording:
            if (event->buttonId == 0 || event->buttonId == 1) {
                // Any button — stop recording
                stopRecording();
            }
            break;

        case Screen::Library:
            if (event->buttonId == 0) {
                // Boot — navigate down
                if (recordingCount_ > 0) {
                    librarySelected_ = (librarySelected_ + 1) % recordingCount_;
                    // Adjust scroll
                    if (librarySelected_ >= libraryScrollTop_ + kLibraryVisibleRows) {
                        libraryScrollTop_ = librarySelected_ - kLibraryVisibleRows + 1;
                    } else if (librarySelected_ < libraryScrollTop_) {
                        libraryScrollTop_ = librarySelected_;
                    }
                }
            } else if (event->buttonId == 1) {
                // Power — play selected
                if (recordingCount_ > 0) {
                    startPlayback(librarySelected_);
                }
            }
            break;

        case Screen::Playing:
            if (event->buttonId == 0 || event->buttonId == 1) {
                stopPlayback();
            }
            break;

        case Screen::ConfirmDelete:
            if (event->buttonId == 1) {
                // Power — confirm delete
                deleteRecording(deleteIndex_);
                scanRecordings();
                if (librarySelected_ >= recordingCount_ && recordingCount_ > 0) {
                    librarySelected_ = recordingCount_ - 1;
                }
                goToScreen(Screen::Library);
            } else if (event->buttonId == 0) {
                // Boot — cancel
                goToScreen(Screen::Library);
            }
            break;

        default:
            break;
    }
}

void DictaphoneCore::handleTouch(const PluginTouchEvent* event) {
    if (!event) return;
    // Only handle touch end (tap)
    if (event->phase != 2) return;

    const uint16_t x = event->x;
    const uint16_t y = event->y;

    switch (screen_) {
        case Screen::Main: {
            // Top half = record, bottom half = library
            int height = display_ && display_->logicalHeight ? display_->logicalHeight() : 172;
            if (y < static_cast<uint16_t>(height / 2)) {
                startRecording();
            } else {
                goToScreen(Screen::Library);
            }
            break;
        }

        case Screen::Recording: {
            // Tap anywhere to stop
            stopRecording();
            break;
        }

        case Screen::Library: {
            if (recordingCount_ == 0) {
                // No recordings — tap anywhere to go back
                goToScreen(Screen::Main);
                return;
            }

            int height = display_ && display_->logicalHeight ? display_->logicalHeight() : 172;
            int width = display_ && display_->logicalWidth ? display_->logicalWidth() : 640;
            uint16_t rowHeight = static_cast<uint16_t>(height / kLibraryVisibleRows);
            uint8_t tappedRow = static_cast<uint8_t>(y / rowHeight);
            uint8_t tappedIndex = libraryScrollTop_ + tappedRow;

            // Right edge = delete
            if (x > static_cast<uint16_t>(width - 120) && tappedIndex < recordingCount_) {
                deleteIndex_ = tappedIndex;
                goToScreen(Screen::ConfirmDelete);
                return;
            }

            // Tap on row = play
            if (tappedIndex < recordingCount_) {
                librarySelected_ = tappedIndex;
                startPlayback(tappedIndex);
            }
            break;
        }

        case Screen::Playing: {
            // Tap anywhere to stop
            stopPlayback();
            break;
        }

        case Screen::ConfirmDelete: {
            int width = display_ && display_->logicalWidth ? display_->logicalWidth() : 640;
            // Left half = cancel, right half = confirm
            if (x < static_cast<uint16_t>(width / 2)) {
                goToScreen(Screen::Library);
            } else {
                deleteRecording(deleteIndex_);
                scanRecordings();
                if (librarySelected_ >= recordingCount_ && recordingCount_ > 0) {
                    librarySelected_ = recordingCount_ - 1;
                }
                goToScreen(Screen::Library);
            }
            break;
        }

        default:
            break;
    }
}

void DictaphoneCore::draw() {
    if (!display_) return;

    switch (screen_) {
        case Screen::Main:          drawMain(); break;
        case Screen::Recording:     drawRecording(); break;
        case Screen::Library:       drawLibrary(); break;
        case Screen::Playing:       drawPlaying(); break;
        case Screen::Rename:        drawRename(); break;
        case Screen::ConfirmDelete: drawConfirmDelete(); break;
    }
}

// ─── Screen Drawing ─────────────────────────────────────────────────────────

void DictaphoneCore::drawMain() {
    if (display_->renderStatus) {
        char countBuf[32];
        if (recordingCount_ > 0) {
            snprintf(countBuf, sizeof(countBuf), "%d recordings | Tap bottom: Library", recordingCount_);
        } else {
            snprintf(countBuf, sizeof(countBuf), "Tap top: Record | Tap bottom: Library");
        }
        display_->renderStatus("DICTAPHONE", "Tap to record", countBuf);
    }
}

void DictaphoneCore::drawRecording() {
    if (!display_->renderProgress) return;

    char timeBuf[8];
    uint32_t elapsed = 0;
    if (audio_ && audio_->recordingElapsedMs) {
        elapsed = audio_->recordingElapsedMs();
    }
    formatTime(elapsed, timeBuf, sizeof(timeBuf));

    // Progress as percentage of 30 min
    int progress = static_cast<int>((elapsed * 100UL) / (30UL * 60UL * 1000UL));
    if (progress > 100) progress = 100;

    display_->renderProgress("RECORDING", timeBuf, "Tap to stop", progress);
}

void DictaphoneCore::drawLibrary() {
    if (!display_->renderMenu) return;

    if (recordingCount_ == 0) {
        if (display_->renderStatus) {
            display_->renderStatus("LIBRARY", "No recordings", "< Back");
        }
        return;
    }

    // Build menu items from visible range
    const char* items[kLibraryVisibleRows];
    uint8_t visibleCount = 0;

    for (uint8_t i = 0; i < kLibraryVisibleRows && (libraryScrollTop_ + i) < recordingCount_; i++) {
        items[i] = recordingNames_[libraryScrollTop_ + i];
        visibleCount++;
    }

    uint8_t selectedInView = librarySelected_ - libraryScrollTop_;
    display_->renderMenu(items, visibleCount, selectedInView);
}

void DictaphoneCore::drawPlaying() {
    if (!display_->renderProgress) return;

    char timeBuf[8];
    char totalBuf[16];
    uint32_t elapsed = 0;
    uint32_t total = 0;

    if (audio_) {
        if (audio_->playbackElapsedMs) elapsed = audio_->playbackElapsedMs();
        if (audio_->playbackTotalMs) total = audio_->playbackTotalMs();
    }

    formatTime(elapsed, timeBuf, sizeof(timeBuf));

    char elapsedTotal[24];
    char totalStr[8];
    formatTime(total, totalStr, sizeof(totalStr));
    snprintf(elapsedTotal, sizeof(elapsedTotal), "%s / %s", timeBuf, totalStr);

    int progress = (total > 0) ? static_cast<int>((elapsed * 100UL) / total) : 0;

    const char* name = (playingIndex_ < recordingCount_)
        ? recordingNames_[playingIndex_] : "---";

    display_->renderProgress("PLAYING", name, elapsedTotal, progress);
}

void DictaphoneCore::drawRename() {
    if (display_->renderStatus) {
        display_->renderStatus("RENAME", renameBuffer_, "");
    }
}

void DictaphoneCore::drawConfirmDelete() {
    if (!display_->renderStatus) return;

    const char* name = (deleteIndex_ < recordingCount_)
        ? recordingNames_[deleteIndex_] : "---";

    display_->renderStatus("DELETE?", name, "Boot=No  Power=Yes");
}

// ─── Recording Actions ──────────────────────────────────────────────────────

void DictaphoneCore::startRecording() {
    if (!audio_ || !audio_->startRecording) {
        // Show error if audio service not available
        if (display_ && display_->renderStatus) {
            display_->renderStatus("ERROR", "Mic not available", "");
        }
        return;
    }

    char filename[kDictMaxFilenameLen];
    if (!generateFilename(filename, sizeof(filename))) return;

    char path[kDictMaxFilenameLen + 16];
    snprintf(path, sizeof(path), "recordings/%s", filename);

    if (audio_->startRecording(path)) {
        // Remember filename for index update after stop
        strncpy(currentRecordingName_, filename, kDictMaxFilenameLen - 1);
        currentRecordingName_[kDictMaxFilenameLen - 1] = '\0';
        goToScreen(Screen::Recording);
    } else {
        // Recording failed to start — show feedback
        if (display_ && display_->renderStatus) {
            display_->renderStatus("ERROR", "Recording failed", "Try again");
        }
    }
}

void DictaphoneCore::stopRecording() {
    if (!audio_ || !audio_->stopRecording) return;

    audio_->stopRecording();

    // Add the newly recorded file to our recordings list and index
    if (currentRecordingName_[0] != '\0' && recordingCount_ < kDictMaxRecordings) {
        strncpy(recordingNames_[recordingCount_], currentRecordingName_, kDictMaxFilenameLen - 1);
        recordingNames_[recordingCount_][kDictMaxFilenameLen - 1] = '\0';
        recordingCount_++;
        saveIndex();
    }
    currentRecordingName_[0] = '\0';

    goToScreen(Screen::Library);
}

void DictaphoneCore::startPlayback(uint8_t index) {
    if (!audio_ || !audio_->startPlayback) return;
    if (index >= recordingCount_) return;

    char path[kDictMaxFilenameLen + 16];
    snprintf(path, sizeof(path), "recordings/%s", recordingNames_[index]);

    if (audio_->startPlayback(path)) {
        playingIndex_ = index;
        goToScreen(Screen::Playing);
    }
}

void DictaphoneCore::stopPlayback() {
    if (!audio_ || !audio_->stopPlayback) return;
    audio_->stopPlayback();
    goToScreen(Screen::Library);
}

// ─── File Management ────────────────────────────────────────────────────────

bool DictaphoneCore::scanRecordings() {
    recordingCount_ = 0;

    if (!storage_ || !storage_->readFile) return false;

    // Read directory listing from a special index file or scan pattern
    // Since PluginStorageService doesn't have listDir, we use a counter-based approach:
    // scan for files named REC_0001.wav through REC_9999.wav
    // Also maintain an index file for faster lookup.

    // Try reading index file first
    uint8_t indexBuf[2048];
    int bytesRead = storage_->readFile("recordings/index.txt", indexBuf, sizeof(indexBuf) - 1);

    if (bytesRead > 0) {
        indexBuf[bytesRead] = '\0';
        // Parse index: one filename per line
        char* line = strtok(reinterpret_cast<char*>(indexBuf), "\n");
        while (line && recordingCount_ < kDictMaxRecordings) {
            // Trim whitespace
            while (*line == ' ' || *line == '\r') line++;
            size_t len = strlen(line);
            while (len > 0 && (line[len-1] == ' ' || line[len-1] == '\r' || line[len-1] == '\n')) {
                line[--len] = '\0';
            }
            if (len > 0 && len < kDictMaxFilenameLen) {
                // Verify file still exists
                char checkPath[kDictMaxFilenameLen + 16];
                snprintf(checkPath, sizeof(checkPath), "recordings/%s", line);
                if (storage_->fileExists && storage_->fileExists(checkPath)) {
                    strncpy(recordingNames_[recordingCount_], line, kDictMaxFilenameLen - 1);
                    recordingNames_[recordingCount_][kDictMaxFilenameLen - 1] = '\0';
                    recordingCount_++;
                }
            }
            line = strtok(nullptr, "\n");
        }
        return true;
    }

    // No index file — no recordings yet (first run)
    // Don't scan 9999 files — just start with empty list
    return true;
}

bool DictaphoneCore::generateFilename(char* buf, size_t bufSize) {
    recordingCounter_++;
    if (recordingCounter_ > 9999) recordingCounter_ = 1;  // wrap

    snprintf(buf, bufSize, "REC_%04u.wav", recordingCounter_);
    return true;
}

bool DictaphoneCore::deleteRecording(uint8_t index) {
    if (index >= recordingCount_) return false;
    if (!storage_ || !storage_->deleteFile) return false;

    char path[kDictMaxFilenameLen + 16];
    snprintf(path, sizeof(path), "recordings/%s", recordingNames_[index]);

    bool result = storage_->deleteFile(path);
    if (result) {
        // Update index
        saveIndex();
    }
    return result;
}

bool DictaphoneCore::renameRecording(uint8_t index, const char* newName) {
    // PluginStorageService doesn't support rename directly, so we'd need
    // read → write → delete. For now, rename is UI-only (rename in index).
    if (index >= recordingCount_ || !newName) return false;

    // Just update the display name in index (file stays the same on SD)
    // This is a simplification — true file rename would need additional API
    strncpy(recordingNames_[index], newName, kDictMaxFilenameLen - 1);
    recordingNames_[index][kDictMaxFilenameLen - 1] = '\0';
    saveIndex();
    return true;
}

void DictaphoneCore::goToScreen(Screen screen) {
    screen_ = screen;
}

void DictaphoneCore::formatTime(uint32_t ms, char* buf, size_t bufSize) {
    uint32_t totalSec = ms / 1000;
    uint32_t minutes = totalSec / 60;
    uint32_t seconds = totalSec % 60;
    snprintf(buf, bufSize, "%02u:%02u", (unsigned)minutes, (unsigned)seconds);
}

// ─── Index File Management ──────────────────────────────────────────────────

void DictaphoneCore::saveIndex() {
    if (!storage_ || !storage_->writeFile) return;

    // Build index content
    char indexBuf[2048];
    size_t offset = 0;

    for (uint8_t i = 0; i < recordingCount_ && offset < sizeof(indexBuf) - kDictMaxFilenameLen - 2; i++) {
        size_t len = strlen(recordingNames_[i]);
        memcpy(indexBuf + offset, recordingNames_[i], len);
        offset += len;
        indexBuf[offset++] = '\n';
    }

    storage_->writeFile("recordings/index.txt",
                        reinterpret_cast<const uint8_t*>(indexBuf),
                        static_cast<uint32_t>(offset));
}

// ─── Plugin SDK VTable Glue ─────────────────────────────────────────────────

static PluginResult dictaphoneInit(PluginContext* ctx) {
    s_instance = new DictaphoneCore(ctx->display, ctx->audio, ctx->storage);
    if (!s_instance) return PLUGIN_ERROR_MEMORY;
    if (!s_instance->begin()) {
        delete s_instance;
        s_instance = nullptr;
        return PLUGIN_ERROR_INIT;
    }
    return PLUGIN_OK;
}

static void dictaphoneDestroy() {
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

static void dictaphoneUpdate(uint32_t nowMs) {
    if (s_instance) s_instance->update(nowMs);
}

static void dictaphoneHandleButton(const PluginButtonEvent* event) {
    if (s_instance) s_instance->handleButton(event);
}

static void dictaphoneHandleTouch(const PluginTouchEvent* event) {
    if (s_instance) s_instance->handleTouch(event);
}

static void dictaphoneDraw() {
    if (s_instance) s_instance->draw();
}

static PluginInfo dictaphoneGetInfo() {
    return {"Dictaphone", "1.0.0", PLUGIN_SDK_VERSION};
}

PluginVTable DictaphonePlugin::vtable() {
    return {
        dictaphoneInit,
        dictaphoneDestroy,
        dictaphoneUpdate,
        dictaphoneHandleButton,
        dictaphoneHandleTouch,
        dictaphoneDraw,
        dictaphoneGetInfo,
    };
}
