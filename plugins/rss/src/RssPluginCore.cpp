// plugins/rss/src/RssPluginCore.cpp
#include "RssPluginCore.h"

// Simple helpers — no standard library available in PIC plugin context
static uint16_t strLen(const char* s) {
    uint16_t len = 0;
    while (s[len] != '\0') ++len;
    return len;
}

static void strCopy(char* dst, const char* src, uint16_t maxLen) {
    uint16_t i = 0;
    while (i < maxLen - 1 && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static bool strEqual(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        ++a;
        ++b;
    }
    return *a == *b;
}

static void intToStr(uint32_t val, char* buf, uint16_t bufSize) {
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

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

RssPluginCore::RssPluginCore(PluginDisplayService* display,
                             PluginStorageService* storage)
    : display_(display), storage_(storage) {
    // Zero out arrays
    for (uint8_t i = 0; i < kMaxFeeds; ++i) feedUrls_[i][0] = '\0';
    for (uint8_t i = 0; i < kMaxArticles; ++i) {
        articleTitles_[i][0] = '\0';
        articleFilenames_[i][0] = '\0';
    }
    articleContent_[0] = '\0';
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

bool RssPluginCore::begin() {
    // Ensure articles directory exists
    storage_->mkdir("articles");

    // Load feed configuration
    loadConfig();

    // Load article list from articles/ directory
    loadArticleList();

    return true;
}

void RssPluginCore::update(uint32_t nowMs) {
    // No periodic work needed for the skeleton.
    // In the future, this could auto-refresh feeds on a timer.
    (void)nowMs;
}

void RssPluginCore::handleButton(const PluginButtonEvent* event) {
    if (!event->pressed) return;  // Only act on press

    if (event->buttonId == 0) {
        // Boot button — navigate down / scroll
        navigateDown();
    } else if (event->buttonId == 1) {
        // Power button — go back / exit
        goBack();
    }
}

void RssPluginCore::handleTouch(const PluginTouchEvent* event) {
    if (event->phase != 2) return;  // Only act on touch end

    int screenHeight = display_->logicalHeight ? display_->logicalHeight() : 300;
    int touchZone = screenHeight / 3;

    if (event->y < (uint16_t)touchZone) {
        // Top third — navigate up
        navigateUp();
    } else if (event->y > (uint16_t)(screenHeight - touchZone)) {
        // Bottom third — navigate down
        navigateDown();
    } else {
        // Middle — select / confirm
        selectItem();
    }
}

void RssPluginCore::draw() {
    switch (state_) {
        case State::ArticleList:
            drawArticleList();
            break;
        case State::ArticleReader:
            drawArticleReader();
            break;
        case State::FeedConfig:
            drawFeedConfig();
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Navigation
// ─────────────────────────────────────────────────────────────────────────────

void RssPluginCore::navigateUp() {
    if (selectedIndex_ > 0) {
        --selectedIndex_;
    }
}

void RssPluginCore::navigateDown() {
    switch (state_) {
        case State::ArticleList: {
            // articleCount items + "Settings" + "Refresh"
            uint8_t maxIndex = articleCount_ + 1;  // 2 extra items, 0-indexed
            if (selectedIndex_ < maxIndex) ++selectedIndex_;
            break;
        }
        case State::ArticleReader: {
            // Scroll content down
            uint16_t linesVisible = 6;  // Approximate lines visible on e-ink
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
                // Open article
                if (loadArticleContent(selectedIndex_)) {
                    state_ = State::ArticleReader;
                    scrollOffset_ = 0;
                }
            } else if (selectedIndex_ == articleCount_) {
                // "Settings" item
                state_ = State::FeedConfig;
                selectedIndex_ = 0;
            } else if (selectedIndex_ == articleCount_ + 1) {
                // "Refresh" item
                refreshFeeds();
                loadArticleList();
            }
            break;
        }
        case State::ArticleReader:
            // Tap in reader scrolls or does nothing special
            break;
        case State::FeedConfig: {
            if (selectedIndex_ < feedCount_) {
                // Remove feed at this index
                for (uint8_t i = selectedIndex_; i < feedCount_ - 1; ++i) {
                    strCopy(feedUrls_[i], feedUrls_[i + 1], kMaxTitleLen);
                }
                feedUrls_[feedCount_ - 1][0] = '\0';
                --feedCount_;
                saveConfig();
            } else if (selectedIndex_ == feedCount_) {
                // "Add Feed" — placeholder (would need text input)
                // For now, add a default example feed if space available
                if (feedCount_ < kMaxFeeds) {
                    strCopy(feedUrls_[feedCount_],
                            "https://example.com/feed.xml", kMaxTitleLen);
                    ++feedCount_;
                    saveConfig();
                }
            } else if (selectedIndex_ == feedCount_ + 1) {
                // "Refresh Feeds"
                refreshFeeds();
                loadArticleList();
            } else {
                // "Back"
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
            // Already at top level — do nothing (exit handled by firmware)
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Drawing
// ─────────────────────────────────────────────────────────────────────────────

void RssPluginCore::drawArticleList() {
    if (!display_->renderMenu) return;

    // Build menu items: article titles + "Settings" + "Refresh"
    uint8_t totalItems = articleCount_ + 2;
    if (totalItems > kMaxArticles) totalItems = kMaxArticles;

    const char* items[kMaxArticles + 2];
    for (uint8_t i = 0; i < articleCount_; ++i) {
        items[i] = articleTitles_[i];
    }
    items[articleCount_] = "[Settings]";
    items[articleCount_ + 1] = "[Refresh]";

    display_->renderMenu(items, totalItems, selectedIndex_);
}

void RssPluginCore::drawArticleReader() {
    if (!display_->renderStatus) return;

    // Show article title and a portion of content
    // renderStatus provides title + 2 lines — use it for simple display
    const char* title = (selectedIndex_ < articleCount_)
                            ? articleTitles_[selectedIndex_]
                            : "Article";

    // Extract a readable chunk from scrollOffset_
    const char* contentPtr = articleContent_;
    uint16_t len = strLen(articleContent_);

    // Simple line-based scroll: skip scrollOffset_ characters
    uint16_t offset = scrollOffset_ < len ? scrollOffset_ : 0;
    const char* line1 = contentPtr + offset;

    // Find a reasonable break for line2
    const char* line2 = "";
    uint16_t lineLen = 40;  // Approximate chars per line on e-ink
    if (offset + lineLen < len) {
        // We can't easily split — just show from offset as line1
        // and the next chunk as line2 using a static buffer
        static char l1Buf[80];
        static char l2Buf[80];
        uint16_t i = 0;
        while (i < 79 && (offset + i) < len && contentPtr[offset + i] != '\n') {
            l1Buf[i] = contentPtr[offset + i];
            ++i;
        }
        l1Buf[i] = '\0';
        // Skip newline
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

    // Items: feed URLs + "Add Feed" + "Refresh" + "Back"
    uint8_t totalItems = feedCount_ + kConfigExtraItems;
    const char* items[kMaxFeeds + kConfigExtraItems];

    for (uint8_t i = 0; i < feedCount_; ++i) {
        items[i] = feedUrls_[i];
    }
    items[feedCount_] = "[Add Feed]";
    items[feedCount_ + 1] = "[Refresh Feeds]";
    items[feedCount_ + 2] = "[Back]";

    display_->renderMenu(items, totalItems, selectedIndex_);
}

// ─────────────────────────────────────────────────────────────────────────────
// Config management
// ─────────────────────────────────────────────────────────────────────────────

bool RssPluginCore::loadConfig() {
    uint8_t buf[kConfigBufSize];
    int bytesRead = storage_->readFile("config.json", buf, kConfigBufSize - 1);
    if (bytesRead <= 0) {
        feedCount_ = 0;
        return false;
    }
    buf[bytesRead] = '\0';

    // Minimal JSON parsing: extract "feeds": ["url1", "url2", ...]
    // We look for quoted strings after "feeds" array start
    const char* json = (const char*)buf;
    const char* feedsStart = nullptr;

    // Find "feeds"
    for (int i = 0; i < bytesRead - 5; ++i) {
        if (json[i] == 'f' && json[i + 1] == 'e' && json[i + 2] == 'e' &&
            json[i + 3] == 'd' && json[i + 4] == 's') {
            feedsStart = json + i + 5;
            break;
        }
    }

    if (!feedsStart) return false;

    // Find '[' to start array
    while (*feedsStart && *feedsStart != '[') ++feedsStart;
    if (!*feedsStart) return false;
    ++feedsStart;  // skip '['

    feedCount_ = 0;
    const char* p = feedsStart;
    while (*p && *p != ']' && feedCount_ < kMaxFeeds) {
        // Find opening quote
        while (*p && *p != '"') ++p;
        if (!*p) break;
        ++p;  // skip opening quote

        // Copy until closing quote
        uint16_t i = 0;
        while (*p && *p != '"' && i < kMaxTitleLen - 1) {
            feedUrls_[feedCount_][i++] = *p++;
        }
        feedUrls_[feedCount_][i] = '\0';
        if (*p == '"') ++p;  // skip closing quote

        ++feedCount_;
    }

    return true;
}

bool RssPluginCore::saveConfig() {
    // Build JSON: {"feeds":["url1","url2",...]}
    char buf[kConfigBufSize];
    uint16_t pos = 0;

    const char* prefix = "{\"feeds\":[";
    uint16_t prefixLen = strLen(prefix);
    for (uint16_t i = 0; i < prefixLen && pos < kConfigBufSize - 1; ++i) {
        buf[pos++] = prefix[i];
    }

    for (uint8_t f = 0; f < feedCount_; ++f) {
        if (f > 0 && pos < kConfigBufSize - 1) buf[pos++] = ',';
        if (pos < kConfigBufSize - 1) buf[pos++] = '"';
        uint16_t urlLen = strLen(feedUrls_[f]);
        for (uint16_t i = 0; i < urlLen && pos < kConfigBufSize - 1; ++i) {
            buf[pos++] = feedUrls_[f][i];
        }
        if (pos < kConfigBufSize - 1) buf[pos++] = '"';
    }

    if (pos < kConfigBufSize - 2) {
        buf[pos++] = ']';
        buf[pos++] = '}';
    }
    buf[pos] = '\0';

    return storage_->writeFile("config.json", (const uint8_t*)buf, pos);
}

// ─────────────────────────────────────────────────────────────────────────────
// Article management
// ─────────────────────────────────────────────────────────────────────────────

bool RssPluginCore::loadArticleList() {
    articleCount_ = 0;

    // Scan articles/ directory by checking known filenames
    // Since PluginStorageService doesn't expose directory listing,
    // we probe for article_001.txt through article_032.txt
    for (uint8_t i = 0; i < kMaxArticles; ++i) {
        char path[40];
        char numBuf[4];
        intToStr(i + 1, numBuf, sizeof(numBuf));

        // Build path: "articles/article_NNN.txt"
        uint16_t pos = 0;
        const char* prefix = "articles/article_";
        while (prefix[pos] != '\0') {
            path[pos] = prefix[pos];
            ++pos;
        }
        // Zero-pad to 3 digits
        uint16_t numLen = strLen(numBuf);
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

        // Read first line as title
        uint8_t titleBuf[kMaxTitleLen];
        int read = storage_->readFile(path, titleBuf, kMaxTitleLen - 1);
        if (read <= 0) continue;
        titleBuf[read] = '\0';

        // Extract first line as title
        uint16_t titleEnd = 0;
        while (titleEnd < (uint16_t)read && titleBuf[titleEnd] != '\n') {
            ++titleEnd;
        }
        if (titleEnd >= kMaxTitleLen) titleEnd = kMaxTitleLen - 1;

        for (uint16_t j = 0; j < titleEnd; ++j) {
            articleTitles_[articleCount_][j] = (char)titleBuf[j];
        }
        articleTitles_[articleCount_][titleEnd] = '\0';

        strCopy(articleFilenames_[articleCount_], path, kMaxTitleLen);
        ++articleCount_;
    }

    return articleCount_ > 0;
}

bool RssPluginCore::loadArticleContent(uint8_t index) {
    if (index >= articleCount_) return false;

    int read = storage_->readFile(articleFilenames_[index],
                                  (uint8_t*)articleContent_,
                                  kMaxContentLen - 1);
    if (read <= 0) {
        articleContent_[0] = '\0';
        contentLength_ = 0;
        return false;
    }
    articleContent_[read] = '\0';
    contentLength_ = (uint16_t)read;

    // Skip the title line for reading display
    uint16_t i = 0;
    while (i < contentLength_ && articleContent_[i] != '\n') ++i;
    if (i < contentLength_) ++i;  // skip newline
    scrollOffset_ = i;

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Feed refresh (skeleton)
// ─────────────────────────────────────────────────────────────────────────────

void RssPluginCore::refreshFeeds() {
    // SKELETON: In the future, this will use a WiFi service to:
    // 1. Connect to WiFi
    // 2. For each feed URL in feedUrls_[], HTTP GET the feed XML
    // 3. Parse RSS/Atom XML to extract <title> and <description>/<content>
    // 4. Write each article as articles/article_NNN.txt
    //
    // For now, this function is a no-op placeholder.
    // Articles can be pre-loaded onto the SD card at /plugins/rss/articles/
    // by the firmware or user for testing.

    // Show a brief status message indicating refresh is not yet available
    if (display_->renderStatus) {
        display_->renderStatus("RSS Refresh",
                               "WiFi service not yet",
                               "available in SDK");
    }
}
