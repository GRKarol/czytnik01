// firmware/src/plugins/builtin/RssPlugin.h
#pragma once

#include <stdint.h>

#include "plugins/sdk/PluginSdk.h"
#include "plugins/sdk/PluginDisplayService.h"
#include "plugins/sdk/PluginStorageService.h"

// Maximum limits for the RSS plugin
static constexpr uint8_t kRssMaxFeeds = 8;
static constexpr uint8_t kRssMaxArticles = 32;
static constexpr uint16_t kRssMaxTitleLen = 64;
static constexpr uint16_t kRssMaxContentLen = 1024;
static constexpr uint16_t kRssConfigBufSize = 512;

class RssPluginCore {
 public:
    enum class State : uint8_t {
        ArticleList,
        ArticleReader,
        FeedConfig,
    };

    RssPluginCore(PluginDisplayService* display, PluginStorageService* storage);

    bool begin();
    void update(uint32_t nowMs);
    void handleButton(const PluginButtonEvent* event);
    void handleTouch(const PluginTouchEvent* event);
    void draw();

 private:
    // Config management
    bool loadConfig();
    bool saveConfig();

    // Article management
    bool loadArticleList();
    bool loadArticleContent(uint8_t index);

    // Feed refresh (skeleton — requires WiFi service in future)
    void refreshFeeds();

    // Navigation helpers
    void navigateUp();
    void navigateDown();
    void selectItem();
    void goBack();

    // Draw helpers
    void drawArticleList();
    void drawArticleReader();
    void drawFeedConfig();

    // Device services
    PluginDisplayService* display_;
    PluginStorageService* storage_;

    // State
    State state_ = State::ArticleList;

    // Feed configuration
    uint8_t feedCount_ = 0;
    char feedUrls_[kRssMaxFeeds][kRssMaxTitleLen];

    // Article list
    uint8_t articleCount_ = 0;
    char articleTitles_[kRssMaxArticles][kRssMaxTitleLen];
    char articleFilenames_[kRssMaxArticles][kRssMaxTitleLen];

    // Current selection
    uint8_t selectedIndex_ = 0;

    // Article reader state
    char articleContent_[kRssMaxContentLen];
    uint16_t contentLength_ = 0;
    uint16_t scrollOffset_ = 0;

    // Config menu items (feeds + "Add Feed" + "Refresh" + "Back")
    static constexpr uint8_t kConfigExtraItems = 3;
};

/// Plugin SDK vtable entry points for the RSS built-in plugin.
namespace RssPlugin {
    PluginVTable vtable();
}
