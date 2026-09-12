// firmware/src/plugins/builtin/RssPlugin.h
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <vector>

#include <WString.h>

#include "plugins/sdk/PluginSdk.h"
#include "plugins/sdk/PluginDisplayService.h"
#include "plugins/sdk/PluginStorageService.h"
#include "plugins/sdk/PluginNetworkService.h"

static constexpr uint8_t kRssMaxFeeds = 8;
static constexpr uint8_t kRssMaxArticles = 20;

class RssCore {
 public:
    enum class Screen : uint8_t {
        Main,             // Idle screen — Feeds / Add feed
        FeedList,         // Saved feeds, tap to fetch, delete icon to remove
        ConfirmDeleteFeed,
        AddFeed,          // Character-picker URL entry (same pattern as
                          // DictaphoneCore's rename screen)
        Fetching,         // Waiting on PluginNetworkService
        ArticleList,      // Titles from the last successfully fetched feed
        ArticleReader,    // Wrapped, scrollable article body
        Error,            // Fetch failed — shows the reason, tap to go back
    };

    struct Article {
        String title;
        String link;
        String body;
    };

    RssCore(PluginDisplayService* display, PluginStorageService* storage,
            PluginNetworkService* network);

    bool begin();
    void update(uint32_t nowMs);
    void handleButton(const PluginButtonEvent* event);
    void handleTouch(const PluginTouchEvent* event);
    void draw();

    // Aborts any in-flight fetch so it doesn't keep running after the
    // plugin is unloaded — see DeviceServicesBridge::teardown()'s own
    // cancelFetch() call for the belt-and-suspenders reason this exists at
    // both layers, same as DictaphoneCore::shutdown() for AudioRecorder.
    void shutdown();

 private:
    void goToScreen(Screen screen);

    // Feed management
    bool loadFeeds();
    void saveFeeds();
    void startFetch(uint8_t feedIndex);

    // Add-feed character picker (mirrors DictaphoneCore's rename screen)
    void openAddFeed();
    void appendUrlChar(char c);
    void handleAddFeedTouch(const PluginTouchEvent* event);

    // Article list (renderMenu-based, mirrors the rename screen's hit-test)
    void handleArticleListTouch(const PluginTouchEvent* event);

    // Article reader
    void openArticle(uint8_t index);
    void handleArticleReaderTouch(const PluginTouchEvent* event);

    // Feed list (renderDeletableList-based, mirrors DictaphoneCore::Library)
    void handleFeedListTouch(const PluginTouchEvent* event);

    void drawMain();
    void drawFeedList();
    void drawConfirmDeleteFeed();
    void drawAddFeed();
    void drawFetching();
    void drawArticleList();
    void drawArticleReader();
    void drawError();

    PluginDisplayService* display_;
    PluginStorageService* storage_;
    PluginNetworkService* network_;

    Screen screen_ = Screen::Main;

    // Saved feed URLs
    uint8_t feedCount_ = 0;
    String feedUrls_[kRssMaxFeeds];
    uint8_t feedListSelected_ = 0;
    uint8_t deleteFeedIndex_ = 0;

    // In-flight / last-fetched feed
    uint8_t fetchingFeedIndex_ = 0;
    String feedTitle_;
    std::vector<Article> articles_;
    uint8_t articleListSelected_ = 0;
    uint8_t openArticleIndex_ = 0;
    int articleScrollLine_ = 0;
    int articleTotalLines_ = 0;
    String errorMessage_;

    // Add-feed URL entry buffer
    String urlBuffer_;
    uint8_t urlKeySelected_ = 0;

    // Touch bookkeeping — same swipe-vs-tap split used by every other
    // touch-driven builtin plugin screen (see DictaphoneCore::touchStartX_/
    // touchStartY_ for the original reasoning).
    uint16_t touchStartX_ = 0;
    uint16_t touchStartY_ = 0;

    uint32_t lastActionMs_ = 0;
    static constexpr uint32_t kActionCooldownMs = 350;
};

/// Plugin SDK vtable entry points for the RSS reader built-in plugin.
namespace RssPlugin {
PluginVTable vtable();
}
