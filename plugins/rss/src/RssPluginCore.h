// plugins/rss/src/RssPluginCore.h
#pragma once

#include "PluginSdk.h"
#include "PluginDisplayService.h"
#include "PluginStorageService.h"

// Maximum limits for the RSS plugin
static constexpr uint8_t kMaxFeeds = 8;
static constexpr uint8_t kMaxArticles = 32;
static constexpr uint16_t kMaxTitleLen = 64;
static constexpr uint16_t kMaxContentLen = 1024;
static constexpr uint16_t kConfigBufSize = 512;

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
    char feedUrls_[kMaxFeeds][kMaxTitleLen];

    // Article list
    uint8_t articleCount_ = 0;
    char articleTitles_[kMaxArticles][kMaxTitleLen];
    char articleFilenames_[kMaxArticles][kMaxTitleLen];

    // Current selection
    uint8_t selectedIndex_ = 0;

    // Article reader state
    char articleContent_[kMaxContentLen];
    uint16_t contentLength_ = 0;
    uint16_t scrollOffset_ = 0;

    // Config menu items (feeds + "Add Feed" + "Refresh" + "Back")
    static constexpr uint8_t kConfigExtraItems = 3;
};
