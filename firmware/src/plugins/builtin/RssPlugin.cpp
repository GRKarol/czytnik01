// firmware/src/plugins/builtin/RssPlugin.cpp
#include "plugins/builtin/RssPlugin.h"

// Simple helpers
static uint16_t rssStrLen(const char* s) {
    uint16_t len = 0;
    while (s[len] != '\0') ++len;
    return len;
}

static void rssStrCopy(char* dst, const char* src, uint16_t maxLen) {
    uint16_t i = 0;
    while (i < maxLen - 1 && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static void rssIntToStr(uint32_t val, char* buf, uint16_t bufSize) {
    if (bufSize == 0) return;
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    char tmp[12];
    uint8_t i = 0;
    while (val > 0 && i < 11) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }
    uint8_t j = 0;
    while (i > 0 && j < bufSize - 1) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

namespace {

// Singleton instance
RssPluginCore* s_rssInstance = nullptr;

}  // namespace

// ─── RssPluginCore Implementation ───────────────────────────────────────────

RssPluginCore::RssPluginCore(PluginDisplayService* display,
                             PluginStorageService* storage)
    : display_(display), storage_(storage) {
    for (uint8_t i = 0; i < kRssMaxFeeds; ++i) feedUrls_[i][0] = '\0';
    for (uint8_t i = 0; i < kRssMaxArticles; ++i) {
        articleTitles_[i][0] = '\0';
        articleFilenames_[i][0] = '\0';
    }
    articleContent_[0] = '\0';
}

bool RssPluginCore::begin() {
    storage_->mkdir("articles");
    loadConfig();
    loadArticleList();
    return true;
}

void RssPluginCore::update(uint32_t nowMs) {
    (void)nowMs;
}

void RssPluginCore::handleButton(const PluginButtonEvent* event) {
    if (!event->pressed) return;

    if (event->buttonId == 0) {
        navigateDown();
    } else if (event->buttonId == 1) {
        goBack();
    }
}

void RssPluginCore::handleTouch(const PluginTouchEvent* event) {
    if (event->phase != 2) return;

    int screenHeight = display_->logicalHeight ? display_->logicalHeight() : 300;
    int touchZone = screenHeight / 3;

    if (event->y < (uint16_t)touchZone) {
        navigateUp();
    } else if (event->y > (uint16_t)(screenHeight - touchZone)) {
        navigateDown();
    } else {
        selectItem();
    }
}

void RssPluginCore::draw() {
    switch (state_) {
        case State::ArticleList: drawArticleList(); break;
        case State::ArticleReader: drawArticleReader(); break;
        case State::FeedConfig: drawFeedConfig(); break;
    }
}

// ─── Navigation ─────────────────────────────────────────────────────────────

void RssPluginCore::navigateUp() {
    if (selectedIndex_ > 0) --selectedIndex_;
}

void RssPluginCore::navigateDown() {
    switch (state_) {
        case State::ArticleList: {
            uint8_t maxIndex = articleCount_ + 1;
            if (selectedIndex_ < maxIndex) ++selectedIndex_;
            break;
        }
        case State::ArticleReader: {
            uint16_t linesVisible = 6;
            if (scrollOffset_ + linesVisible < contentLength_) {
                scrollOffset_ += linesVisible;
            }
            break;
        }
        case State::FeedConfig: {
            uint8_t maxIndex = feedCount_ + kConfigExtraItems - 1;
            if (selectedIndex_ < maxIndex) ++selectedIndex_;
            break;
        }
    }
}

void RssPluginCore::selectItem() {
    switch (state_) {
        case State::ArticleList: {
            if (selectedIndex_ < articleCount_) {
                if (loadArticleContent(selectedIndex_)) {
                    state_ = State::ArticleReader;
                    scrollOffset_ = 0;
                }
            } else if (selectedIndex_ == articleCount_) {
                state_ = State::FeedConfig;
                selectedIndex_ = 0;
            } else if (selectedIndex_ == articleCount_ + 1) {
                refreshFeeds();
                loadArticleList();
            }
            break;
        }
        case State::ArticleReader:
            break;
        case State::FeedConfig: {
            if (selectedIndex_ < feedCount_) {
                for (uint8_t i = selectedIndex_; i < feedCount_ - 1; ++i) {
                    rssStrCopy(feedUrls_[i], feedUrls_[i + 1], kRssMaxTitleLen);
                }
                feedUrls_[feedCount_ - 1][0] = '\0';
                --feedCount_;
                saveConfig();
            } else if (selectedIndex_ == feedCount_) {
                if (feedCount_ < kRssMaxFeeds) {
                    rssStrCopy(feedUrls_[feedCount_],
                               "https://example.com/feed.xml", kRssMaxTitleLen);
                    ++feedCount_;
                    saveConfig();
                }
            } else if (selectedIndex_ == feedCount_ + 1) {
                refreshFeeds();
                loadArticleList();
            } else {
                state_ = State::ArticleList;
                selectedIndex_ = 0;
            }
            break;
        }
    }
}

void RssPluginCore::goBack() {
    switch (state_) {
        case State::ArticleReader:
            state_ = State::ArticleList;
            scrollOffset_ = 0;
            break;
        case State::FeedConfig:
            state_ = State::ArticleList;
            selectedIndex_ = 0;
            break;
        case State::ArticleList:
            break;
    }
}

// ─── Drawing ────────────────────────────────────────────────────────────────

void RssPluginCore::drawArticleList() {
    if (!display_->renderMenu) return;

    uint8_t totalItems = articleCount_ + 2;
    if (totalItems > kRssMaxArticles) totalItems = kRssMaxArticles;

    const char* items[kRssMaxArticles + 2];
    for (uint8_t i = 0; i < articleCount_; ++i) {
        items[i] = articleTitles_[i];
    }
    items[articleCount_] = "[Settings]";
    items[articleCount_ + 1] = "[Refresh]";

    display_->renderMenu(items, totalItems, selectedIndex_);
}

void RssPluginCore::drawArticleReader() {
    if (!display_->renderStatus) return;

    const char* title = (selectedIndex_ < articleCount_)
                            ? articleTitles_[selectedIndex_]
                            : "Article";

    const char* contentPtr = articleContent_;
    uint16_t len = rssStrLen(articleContent_);
    uint16_t offset = scrollOffset_ < len ? scrollOffset_ : 0;

    const char* line1 = contentPtr + offset;
    const char* line2 = "";
    uint16_t lineLen = 40;

    if (offset + lineLen < len) {
        static char l1Buf[80];
        static char l2Buf[80];
        uint16_t i = 0;
        while (i < 79 && (offset + i) < len && contentPtr[offset + i] != '\n') {
            l1Buf[i] = contentPtr[offset + i];
            ++i;
        }
        l1Buf[i] = '\0';
        uint16_t start2 = offset + i;
        if (start2 < len && contentPtr[start2] == '\n') ++start2;
        uint16_t j = 0;
        while (j < 79 && (start2 + j) < len && contentPtr[start2 + j] != '\n') {
            l2Buf[j] = contentPtr[start2 + j];
            ++j;
        }
        l2Buf[j] = '\0';
        line1 = l1Buf;
        line2 = l2Buf;
    }

    display_->renderStatus(title, line1, line2);
}

void RssPluginCore::drawFeedConfig() {
    if (!display_->renderMenu) return;

    uint8_t totalItems = feedCount_ + kConfigExtraItems;
    const char* items[kRssMaxFeeds + kConfigExtraItems];

    for (uint8_t i = 0; i < feedCount_; ++i) {
        items[i] = feedUrls_[i];
    }
    items[feedCount_] = "[Add Feed]";
    items[feedCount_ + 1] = "[Refresh Feeds]";
    items[feedCount_ + 2] = "[Back]";

    display_->renderMenu(items, totalItems, selectedIndex_);
}

// ─── Config Management ──────────────────────────────────────────────────────

bool RssPluginCore::loadConfig() {
    uint8_t buf[kRssConfigBufSize];
    int bytesRead = storage_->readFile("config.json", buf, kRssConfigBufSize - 1);
    if (bytesRead <= 0) {
        feedCount_ = 0;
        return false;
    }
    buf[bytesRead] = '\0';

    const char* json = (const char*)buf;
    const char* feedsStart = nullptr;

    for (int i = 0; i < bytesRead - 5; ++i) {
        if (json[i] == 'f' && json[i + 1] == 'e' && json[i + 2] == 'e' &&
            json[i + 3] == 'd' && json[i + 4] == 's') {
            feedsStart = json + i + 5;
            break;
        }
    }

    if (!feedsStart) return false;

    while (*feedsStart && *feedsStart != '[') ++feedsStart;
    if (!*feedsStart) return false;
    ++feedsStart;

    feedCount_ = 0;
    const char* p = feedsStart;
    while (*p && *p != ']' && feedCount_ < kRssMaxFeeds) {
        while (*p && *p != '"') ++p;
        if (!*p) break;
        ++p;

        uint16_t i = 0;
        while (*p && *p != '"' && i < kRssMaxTitleLen - 1) {
            feedUrls_[feedCount_][i++] = *p++;
        }
        feedUrls_[feedCount_][i] = '\0';
        if (*p == '"') ++p;

        ++feedCount_;
    }

    return true;
}

bool RssPluginCore::saveConfig() {
    char buf[kRssConfigBufSize];
    uint16_t pos = 0;

    const char* prefix = "{\"feeds\":[";
    uint16_t prefixLen = rssStrLen(prefix);
    for (uint16_t i = 0; i < prefixLen && pos < kRssConfigBufSize - 1; ++i) {
        buf[pos++] = prefix[i];
    }

    for (uint8_t f = 0; f < feedCount_; ++f) {
        if (f > 0 && pos < kRssConfigBufSize - 1) buf[pos++] = ',';
        if (pos < kRssConfigBufSize - 1) buf[pos++] = '"';
        uint16_t urlLen = rssStrLen(feedUrls_[f]);
        for (uint16_t i = 0; i < urlLen && pos < kRssConfigBufSize - 1; ++i) {
            buf[pos++] = feedUrls_[f][i];
        }
        if (pos < kRssConfigBufSize - 1) buf[pos++] = '"';
    }

    if (pos < kRssConfigBufSize - 2) {
        buf[pos++] = ']';
        buf[pos++] = '}';
    }
    buf[pos] = '\0';

    return storage_->writeFile("config.json", (const uint8_t*)buf, pos);
}

// ─── Article Management ─────────────────────────────────────────────────────

bool RssPluginCore::loadArticleList() {
    articleCount_ = 0;

    for (uint8_t i = 0; i < kRssMaxArticles; ++i) {
        char path[40];
        char numBuf[4];
        rssIntToStr(i + 1, numBuf, sizeof(numBuf));

        uint16_t pos = 0;
        const char* pathPrefix = "articles/article_";
        while (pathPrefix[pos] != '\0') {
            path[pos] = pathPrefix[pos];
            ++pos;
        }
        uint16_t numLen = rssStrLen(numBuf);
        for (uint16_t pad = numLen; pad < 3; ++pad) {
            path[pos++] = '0';
        }
        for (uint16_t j = 0; j < numLen; ++j) {
            path[pos++] = numBuf[j];
        }
        const char* suffix = ".txt";
        for (uint16_t j = 0; suffix[j]; ++j) {
            path[pos++] = suffix[j];
        }
        path[pos] = '\0';

        if (!storage_->fileExists(path)) continue;

        uint8_t titleBuf[kRssMaxTitleLen];
        int readBytes = storage_->readFile(path, titleBuf, kRssMaxTitleLen - 1);
        if (readBytes <= 0) continue;
        titleBuf[readBytes] = '\0';

        uint16_t titleEnd = 0;
        while (titleEnd < (uint16_t)readBytes && titleBuf[titleEnd] != '\n') {
            ++titleEnd;
        }
        if (titleEnd >= kRssMaxTitleLen) titleEnd = kRssMaxTitleLen - 1;

        for (uint16_t j = 0; j < titleEnd; ++j) {
            articleTitles_[articleCount_][j] = (char)titleBuf[j];
        }
        articleTitles_[articleCount_][titleEnd] = '\0';

        rssStrCopy(articleFilenames_[articleCount_], path, kRssMaxTitleLen);
        ++articleCount_;
    }

    return articleCount_ > 0;
}

bool RssPluginCore::loadArticleContent(uint8_t index) {
    if (index >= articleCount_) return false;

    int readBytes = storage_->readFile(articleFilenames_[index],
                                       (uint8_t*)articleContent_,
                                       kRssMaxContentLen - 1);
    if (readBytes <= 0) {
        articleContent_[0] = '\0';
        contentLength_ = 0;
        return false;
    }
    articleContent_[readBytes] = '\0';
    contentLength_ = (uint16_t)readBytes;

    uint16_t i = 0;
    while (i < contentLength_ && articleContent_[i] != '\n') ++i;
    if (i < contentLength_) ++i;
    scrollOffset_ = i;

    return true;
}

// ─── Feed Refresh ───────────────────────────────────────────────────────────

void RssPluginCore::refreshFeeds() {
    if (display_->renderStatus) {
        display_->renderStatus("RSS Refresh",
                               "WiFi service not yet",
                               "available in SDK");
    }
}

// ─── Plugin SDK VTable Glue ─────────────────────────────────────────────────

static PluginResult rssInit(PluginContext* ctx) {
    s_rssInstance = new RssPluginCore(ctx->display, ctx->storage);
    if (!s_rssInstance) return PLUGIN_ERROR_MEMORY;
    if (!s_rssInstance->begin()) {
        delete s_rssInstance;
        s_rssInstance = nullptr;
        return PLUGIN_ERROR_INIT;
    }
    return PLUGIN_OK;
}

static void rssDestroy() {
    if (s_rssInstance) {
        delete s_rssInstance;
        s_rssInstance = nullptr;
    }
}

static void rssUpdate(uint32_t nowMs) {
    if (s_rssInstance) s_rssInstance->update(nowMs);
}

static void rssHandleButton(const PluginButtonEvent* event) {
    if (s_rssInstance) s_rssInstance->handleButton(event);
}

static void rssHandleTouch(const PluginTouchEvent* event) {
    if (s_rssInstance) s_rssInstance->handleTouch(event);
}

static void rssDraw() {
    if (s_rssInstance) s_rssInstance->draw();
}

static PluginInfo rssGetInfo() {
    return {"RSS Reader", "1.0.0", PLUGIN_SDK_VERSION};
}

PluginVTable RssPlugin::vtable() {
    return {
        rssInit,
        rssDestroy,
        rssUpdate,
        rssHandleButton,
        rssHandleTouch,
        rssDraw,
        rssGetInfo,
    };
}
