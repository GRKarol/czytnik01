// firmware/src/plugins/builtin/RssPlugin.cpp
#include "plugins/builtin/RssPlugin.h"

#include <string.h>
#include <stdio.h>
#include <algorithm>

namespace {

RssCore* s_instance = nullptr;

constexpr uint8_t kFeedListVisibleRows = 5;
// Mirrors DeviceServicesBridge.cpp's kDeletableListBackZoneWidth/
// kDeletableListIconZoneWidth — the geometry it actually draws with, since
// PluginDisplayService::renderDeletableList doesn't hand hit-test rects
// back to the caller (see DictaphoneCore's Library screen for the same
// reasoning).
constexpr uint16_t kFeedListBackZoneWidth = 64;
constexpr uint16_t kFeedListDeleteZoneWidth = 120;

// Character picker for the Add Feed URL entry — same mechanism as
// DictaphoneCore's rename screen (renderMenu as a scrollable key list),
// just with a charset that actually covers URLs instead of filenames.
constexpr char kUrlKeyChars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    "./:-_?=&%~+#";
constexpr size_t kUrlKeyCharCount = sizeof(kUrlKeyChars) - 1;  // drop '\0'
constexpr uint8_t kUrlActionRowCount = 3;                      // Save, Backspace, Cancel
constexpr size_t kMaxUrlLen = 200;

// DisplayManager::kCompactMenuRowHeight — renderMenu()'s fixed row height.
// Duplicated here for the same reason DictaphoneCore duplicates it for its
// rename screen: a plugin can't reach DisplayManager's private constants,
// and renderMenu's centered-window scroll math has to be replicated
// exactly to hit-test a tap against it.
constexpr int kMenuRowHeight = 22;

// Approximates DisplayManager::renderArticleReader()'s actual visible row
// count ((172 - 24) / 18 ~= 8) for tap-to-page navigation. Doesn't need to
// be exact — worst case a page turn overlaps or gaps by a line or two.
constexpr int kArticleVisibleLinesPerPage = 8;

// ─── Localization ───────────────────────────────────────────────────────────
//
// Kept as the plugin's own copy, same as DictaphoneCore/FocusTimerCore —
// see DictaphonePlugin.cpp's DictStr comment for why this doesn't just
// include app/Translations.h. 0=EN,1=ES,2=FR,3=DE,4=RO,5=PL.
enum class RssStr : uint8_t {
    Rss,
    Feeds,
    AddFeed,
    NoFeeds,
    TapToAddFeed,
    Save,
    Backspace,
    Cancel,
    Delete,
    Connecting,
    Fetching,
    NoArticles,
    NoContent,
    ErrorTitle,
    NoSavedWifi,
    WifiConnectFailed,
    TryAgain,
    TapToGoBack,
};

const char* rssText(RssStr key, int lang) {
    switch (key) {
        case RssStr::Rss:
            switch (lang) {
                case 5: return "RSS";
                default: return "RSS";
            }
        case RssStr::Feeds:
            switch (lang) {
                case 1: return "Fuentes";
                case 2: return "Flux";
                case 3: return "Feeds";
                case 4: return "Fluxuri";
                case 5: return "Kanaly";
                default: return "Feeds";
            }
        case RssStr::AddFeed:
            switch (lang) {
                case 1: return "Anadir fuente";
                case 2: return "Ajouter un flux";
                case 3: return "Feed hinzufuegen";
                case 4: return "Adauga flux";
                case 5: return "Dodaj kanal";
                default: return "Add feed";
            }
        case RssStr::NoFeeds:
            switch (lang) {
                case 1: return "Sin fuentes";
                case 2: return "Aucun flux";
                case 3: return "Keine Feeds";
                case 4: return "Niciun flux";
                case 5: return "Brak kanalow";
                default: return "No feeds";
            }
        case RssStr::TapToAddFeed:
            switch (lang) {
                case 1: return "Toca para anadir";
                case 2: return "Touchez pour ajouter";
                case 3: return "Tippen zum Hinzufuegen";
                case 4: return "Atinge pentru a adauga";
                case 5: return "Dotknij, aby dodac";
                default: return "Tap to add a feed";
            }
        case RssStr::Save:
            switch (lang) {
                case 1: return "Guardar";
                case 2: return "Enregistrer";
                case 3: return "Speichern";
                case 4: return "Salveaza";
                case 5: return "Zapisz";
                default: return "Save";
            }
        case RssStr::Backspace:
            switch (lang) {
                case 1: return "Borrar";
                case 2: return "Effacer";
                case 3: return "Loeschen";
                case 4: return "Sterge";
                case 5: return "Usun znak";
                default: return "Backspace";
            }
        case RssStr::Cancel:
            switch (lang) {
                case 1: return "Cancelar";
                case 2: return "Annuler";
                case 3: return "Abbrechen";
                case 4: return "Anuleaza";
                case 5: return "Anuluj";
                default: return "Cancel";
            }
        case RssStr::Delete:
            switch (lang) {
                case 1: return "Eliminar";
                case 2: return "Supprimer";
                case 3: return "Loeschen";
                case 4: return "Sterge";
                case 5: return "Usun";
                default: return "Delete";
            }
        case RssStr::Connecting:
            switch (lang) {
                case 1: return "Conectando";
                case 2: return "Connexion";
                case 3: return "Verbinde";
                case 4: return "Se conecteaza";
                case 5: return "Laczenie";
                default: return "Connecting";
            }
        case RssStr::Fetching:
            switch (lang) {
                case 1: return "Descargando";
                case 2: return "Telechargement";
                case 3: return "Lade";
                case 4: return "Se descarca";
                case 5: return "Pobieranie";
                default: return "Fetching";
            }
        case RssStr::NoArticles:
            switch (lang) {
                case 1: return "Sin articulos";
                case 2: return "Aucun article";
                case 3: return "Keine Artikel";
                case 4: return "Niciun articol";
                case 5: return "Brak artykulow";
                default: return "No articles";
            }
        case RssStr::NoContent:
            switch (lang) {
                case 1: return "Sin contenido";
                case 2: return "Aucun contenu";
                case 3: return "Kein Inhalt";
                case 4: return "Fara continut";
                case 5: return "Brak tresci";
                default: return "No content";
            }
        case RssStr::ErrorTitle:
            switch (lang) {
                case 1: return "Error";
                case 2: return "Erreur";
                case 3: return "Fehler";
                case 4: return "Eroare";
                case 5: return "Blad";
                default: return "Error";
            }
        case RssStr::NoSavedWifi:
            switch (lang) {
                case 1: return "Sin red WiFi guardada";
                case 2: return "Aucun WiFi enregistre";
                case 3: return "Kein gespeichertes WLAN";
                case 4: return "Nicio retea WiFi salvata";
                case 5: return "Brak zapisanej sieci WiFi";
                default: return "No saved WiFi network";
            }
        case RssStr::WifiConnectFailed:
            switch (lang) {
                case 1: return "Fallo de conexion WiFi";
                case 2: return "Echec de connexion WiFi";
                case 3: return "WLAN-Verbindung fehlgeschlagen";
                case 4: return "Conectare WiFi esuata";
                case 5: return "Nie polaczono z WiFi";
                default: return "WiFi connect failed";
            }
        case RssStr::TryAgain:
            switch (lang) {
                case 1: return "Intentalo de nuevo";
                case 2: return "Reessayez";
                case 3: return "Erneut versuchen";
                case 4: return "Incearca din nou";
                case 5: return "Sprobuj ponownie";
                default: return "Try again";
            }
        case RssStr::TapToGoBack:
            switch (lang) {
                case 1: return "Toca para volver";
                case 2: return "Touchez pour revenir";
                case 3: return "Tippen zum Zurueckgehen";
                case 4: return "Atinge pentru a reveni";
                case 5: return "Dotknij, aby wrocic";
                default: return "Tap to go back";
            }
    }
    return "";
}

// ─── Minimal RSS/Atom parsing ────────────────────────────────────────────────
//
// No XML library in this codebase (see the commit that removed the old
// plugins/rss/ scaffold) — same "hand-rolled, tag-scanning" approach
// CompanionSyncManager.cpp and BleApi.cpp already use for JSON. Handles the
// subset of RSS 2.0 / Atom that real-world feeds actually use: <item>/
// <entry> blocks, <title>, <description>/<content:encoded>/<summary>/
// <content>, and <link> (text node for RSS, href attribute for Atom).

bool tagOpensAt(const String& xml, const String& openNeedle, int pos, size_t* afterNeedle) {
    if (pos < 0) return false;
    const size_t after = static_cast<size_t>(pos) + openNeedle.length();
    const char c = after < xml.length() ? xml[after] : '\0';
    if (c == '>' || c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '/') {
        *afterNeedle = after;
        return true;
    }
    return false;
}

// Finds the next <tagName ...>...</tagName> at or after fromPos and returns
// its inner text. Self-closing tags (<tagName .../>) return an empty inner
// text. Returns false if the tag doesn't appear again in xml.
bool extractTagText(const String& xml, const char* tagName, size_t fromPos, String& outText,
                     size_t& outAfterEnd) {
    const String openNeedle = String("<") + tagName;
    size_t searchFrom = fromPos;

    while (searchFrom <= xml.length()) {
        const int openPos = xml.indexOf(openNeedle, searchFrom);
        if (openPos < 0) return false;

        size_t afterNeedle = 0;
        if (!tagOpensAt(xml, openNeedle, openPos, &afterNeedle)) {
            searchFrom = static_cast<size_t>(openPos) + openNeedle.length();
            continue;
        }

        const int gt = xml.indexOf('>', openPos);
        if (gt < 0) return false;

        if (gt > 0 && xml[gt - 1] == '/') {
            outText = "";
            outAfterEnd = static_cast<size_t>(gt) + 1;
            return true;
        }

        const String closeNeedle = String("</") + tagName + ">";
        const int closePos = xml.indexOf(closeNeedle, gt);
        if (closePos < 0) return false;

        outText = xml.substring(gt + 1, closePos);
        outAfterEnd = static_cast<size_t>(closePos) + closeNeedle.length();
        return true;
    }
    return false;
}

bool extractAttribute(const String& tagContent, const char* attrName, String& outValue) {
    const String needle = String(attrName) + "=\"";
    const int pos = tagContent.indexOf(needle);
    if (pos < 0) return false;
    const int start = pos + static_cast<int>(needle.length());
    const int end = tagContent.indexOf('"', start);
    if (end < 0) return false;
    outValue = tagContent.substring(start, end);
    return true;
}

String stripHtmlTags(const String& input) {
    String out;
    out.reserve(input.length());
    bool inTag = false;
    for (size_t i = 0; i < input.length(); ++i) {
        const char c = input[i];
        if (c == '<') {
            inTag = true;
            continue;
        }
        if (c == '>') {
            inTag = false;
            continue;
        }
        if (!inTag) out += c;
    }
    return out;
}

String xmlUnescapeAndStrip(String text) {
    text.trim();
    if (text.startsWith("<![CDATA[") && text.endsWith("]]>")) {
        text = text.substring(9, text.length() - 3);
        text.trim();
    }
    text.replace("&nbsp;", " ");
    text.replace("&lt;", "<");
    text.replace("&gt;", ">");
    text.replace("&quot;", "\"");
    text.replace("&apos;", "'");
    text.replace("&amp;", "&");  // last — the others never introduce a literal '&'
    return text;
}

void parseFeedBody(const String& xml, std::vector<RssCore::Article>& outArticles,
                    String& outFeedTitle) {
    outArticles.clear();
    outFeedTitle = "";

    const bool isAtom = xml.indexOf("<feed") >= 0 && xml.indexOf("<rss") < 0;
    const char* itemTag = isAtom ? "entry" : "item";

    {
        String rawTitle;
        size_t after;
        if (extractTagText(xml, "title", 0, rawTitle, after)) {
            outFeedTitle = xmlUnescapeAndStrip(rawTitle);
        }
    }

    const String openNeedle = String("<") + itemTag;
    const String closeNeedle = String("</") + itemTag + ">";
    size_t pos = 0;

    while (outArticles.size() < kRssMaxArticles) {
        const int openPos = xml.indexOf(openNeedle, pos);
        if (openPos < 0) break;
        size_t afterNeedle = 0;
        if (!tagOpensAt(xml, openNeedle, openPos, &afterNeedle)) {
            pos = static_cast<size_t>(openPos) + openNeedle.length();
            continue;
        }
        const int itemGt = xml.indexOf('>', openPos);
        if (itemGt < 0) break;
        const int closePos = xml.indexOf(closeNeedle, itemGt);
        if (closePos < 0) break;

        const String block = xml.substring(itemGt + 1, closePos);
        pos = static_cast<size_t>(closePos) + closeNeedle.length();

        RssCore::Article article;
        String rawTitle, rawBody;
        size_t innerAfter;

        if (extractTagText(block, "title", 0, rawTitle, innerAfter)) {
            article.title = xmlUnescapeAndStrip(rawTitle);
        }

        bool haveBody = extractTagText(block, "content:encoded", 0, rawBody, innerAfter);
        if (!haveBody) haveBody = extractTagText(block, "description", 0, rawBody, innerAfter);
        if (!haveBody) haveBody = extractTagText(block, "summary", 0, rawBody, innerAfter);
        if (!haveBody) haveBody = extractTagText(block, "content", 0, rawBody, innerAfter);
        if (haveBody) {
            article.body = stripHtmlTags(xmlUnescapeAndStrip(rawBody));
            article.body.trim();
        }

        if (isAtom) {
            const int linkOpenPos = block.indexOf("<link");
            if (linkOpenPos >= 0) {
                const int linkGt = block.indexOf('>', linkOpenPos);
                if (linkGt >= 0) {
                    const String linkTag = block.substring(linkOpenPos, linkGt + 1);
                    extractAttribute(linkTag, "href", article.link);
                }
            }
        } else {
            String rawLink;
            if (extractTagText(block, "link", 0, rawLink, innerAfter)) {
                article.link = xmlUnescapeAndStrip(rawLink);
            }
        }

        if (article.title.length() > 120) article.title = article.title.substring(0, 120);
        if (article.body.length() > 4000) article.body = article.body.substring(0, 4000);

        if (!article.title.isEmpty() || !article.body.isEmpty()) {
            outArticles.push_back(article);
        }
    }
}

}  // namespace

// ─── RssCore ─────────────────────────────────────────────────────────────────

RssCore::RssCore(PluginDisplayService* display, PluginStorageService* storage,
                  PluginNetworkService* network)
    : display_(display), storage_(storage), network_(network) {}

bool RssCore::begin() {
    loadFeeds();
    return true;
}

void RssCore::shutdown() {
    if (network_ && network_->cancelFetch) {
        network_->cancelFetch();
    }
}

void RssCore::goToScreen(Screen screen) {
    screen_ = screen;
}

void RssCore::update(uint32_t nowMs) {
    (void)nowMs;

    if (screen_ == Screen::Fetching && network_ && network_->fetchStatus) {
        const PluginNetworkStatus status = network_->fetchStatus();
        if (status == PLUGIN_NETWORK_DONE) {
            const char* body = network_->fetchResultBody ? network_->fetchResultBody() : "";
            parseFeedBody(String(body), articles_, feedTitle_);
            articleListSelected_ = 0;
            if (articles_.empty()) {
                errorMessage_ = rssText(RssStr::NoArticles, display_->languageIndex
                                                                 ? display_->languageIndex()
                                                                 : 0);
                goToScreen(Screen::Error);
            } else {
                goToScreen(Screen::ArticleList);
            }
        } else if (status == PLUGIN_NETWORK_ERROR) {
            const char* err = network_->fetchErrorMessage ? network_->fetchErrorMessage() : "";
            errorMessage_ = err;
            goToScreen(Screen::Error);
        }
    }
}

void RssCore::handleButton(const PluginButtonEvent* event) {
    if (!event || !event->pressed) return;
    // Only the boot button (id 0) ever reaches a plugin — see
    // DictaphoneCore::handleButton()'s comment on why every real action
    // must still be reachable by touch alone.
    if (event->buttonId != 0) return;

    switch (screen_) {
        case Screen::Main:
            goToScreen(Screen::FeedList);
            break;
        case Screen::FeedList:
        case Screen::ConfirmDeleteFeed:
            goToScreen(Screen::Main);
            break;
        case Screen::AddFeed:
            goToScreen(Screen::FeedList);
            break;
        case Screen::Fetching:
            if (network_ && network_->cancelFetch) network_->cancelFetch();
            goToScreen(Screen::FeedList);
            break;
        case Screen::ArticleList:
            goToScreen(Screen::FeedList);
            break;
        case Screen::ArticleReader:
            goToScreen(Screen::ArticleList);
            break;
        case Screen::Error:
            goToScreen(Screen::FeedList);
            break;
    }
}

void RssCore::handleTouch(const PluginTouchEvent* event) {
    if (!event) return;

    if (event->phase == 0) {
        touchStartX_ = event->x;
        touchStartY_ = event->y;
    }

    switch (screen_) {
        case Screen::Main: {
            if (event->phase != 2) return;
            if (event->timestampMs - lastActionMs_ < kActionCooldownMs) return;
            lastActionMs_ = event->timestampMs;
            const int width = display_ && display_->logicalWidth ? display_->logicalWidth() : 640;
            if (event->x < static_cast<uint16_t>(width / 2)) {
                goToScreen(Screen::FeedList);
            } else {
                openAddFeed();
            }
            break;
        }
        case Screen::FeedList:
            handleFeedListTouch(event);
            break;
        case Screen::ConfirmDeleteFeed: {
            if (event->phase != 2) return;
            if (event->timestampMs - lastActionMs_ < kActionCooldownMs) return;
            lastActionMs_ = event->timestampMs;
            const int width = display_ && display_->logicalWidth ? display_->logicalWidth() : 640;
            if (event->x < static_cast<uint16_t>(width / 2)) {
                goToScreen(Screen::FeedList);
            } else {
                for (uint8_t i = deleteFeedIndex_; i + 1 < feedCount_; ++i) {
                    feedUrls_[i] = feedUrls_[i + 1];
                }
                if (feedCount_ > 0) feedCount_--;
                saveFeeds();
                if (feedListSelected_ >= feedCount_ && feedCount_ > 0) {
                    feedListSelected_ = feedCount_ - 1;
                }
                goToScreen(Screen::FeedList);
            }
            break;
        }
        case Screen::AddFeed:
            handleAddFeedTouch(event);
            break;
        case Screen::Fetching:
            // Tap anywhere to cancel and go back.
            if (event->phase != 2) return;
            if (network_ && network_->cancelFetch) network_->cancelFetch();
            goToScreen(Screen::FeedList);
            break;
        case Screen::ArticleList:
            handleArticleListTouch(event);
            break;
        case Screen::ArticleReader:
            handleArticleReaderTouch(event);
            break;
        case Screen::Error:
            if (event->phase != 2) return;
            goToScreen(Screen::FeedList);
            break;
    }
}

// ─── Feed list (renderDeletableList) ────────────────────────────────────────

void RssCore::handleFeedListTouch(const PluginTouchEvent* event) {
    if (event->phase != 2) return;
    if (event->timestampMs - lastActionMs_ < kActionCooldownMs) return;
    lastActionMs_ = event->timestampMs;

    // The last synthetic row is always "+ Add feed" — see drawFeedList().
    const uint8_t rowCount = static_cast<uint8_t>(feedCount_ + 1);
    const int height = display_ && display_->logicalHeight ? display_->logicalHeight() : 172;
    const int width = display_ && display_->logicalWidth ? display_->logicalWidth() : 640;

    // Vertical swipe pages the list, same as DictaphoneCore::Library.
    const int deltaX = static_cast<int>(event->x) - static_cast<int>(touchStartX_);
    const int deltaY = static_cast<int>(event->y) - static_cast<int>(touchStartY_);
    const int absDeltaX = deltaX < 0 ? -deltaX : deltaX;
    const int absDeltaY = deltaY < 0 ? -deltaY : deltaY;
    constexpr int kSwipeThresholdPx = 30;
    if (absDeltaY >= kSwipeThresholdPx && absDeltaY > absDeltaX) {
        return;  // Everything fits without scrolling for kRssMaxFeeds=8 rows worth — no-op.
    }

    uint8_t visibleRows = rowCount;
    if (visibleRows > kFeedListVisibleRows) visibleRows = kFeedListVisibleRows;
    if (visibleRows == 0) {
        goToScreen(Screen::Main);
        return;
    }

    const uint16_t rowHeight = static_cast<uint16_t>(height / visibleRows);
    uint8_t tappedRow = static_cast<uint8_t>(event->y / rowHeight);
    if (tappedRow >= visibleRows) tappedRow = visibleRows - 1;
    const uint8_t tappedIndex = tappedRow;

    if (event->x < kFeedListBackZoneWidth) {
        goToScreen(Screen::Main);
        return;
    }

    if (tappedIndex == feedCount_) {
        // "+ Add feed" row.
        openAddFeed();
        return;
    }

    if (tappedIndex >= feedCount_) return;

    if (event->x > static_cast<uint16_t>(width - kFeedListDeleteZoneWidth)) {
        deleteFeedIndex_ = tappedIndex;
        goToScreen(Screen::ConfirmDeleteFeed);
        return;
    }

    feedListSelected_ = tappedIndex;
    startFetch(tappedIndex);
}

void RssCore::startFetch(uint8_t feedIndex) {
    if (feedIndex >= feedCount_) return;
    if (!network_ || !network_->startFetch) return;

    const int lang = display_ && display_->languageIndex ? display_->languageIndex() : 0;

    if (network_->hasSavedNetwork && !network_->hasSavedNetwork()) {
        errorMessage_ = rssText(RssStr::NoSavedWifi, lang);
        goToScreen(Screen::Error);
        return;
    }

    if (!network_->startFetch(feedUrls_[feedIndex].c_str())) {
        errorMessage_ = rssText(RssStr::WifiConnectFailed, lang);
        goToScreen(Screen::Error);
        return;
    }

    fetchingFeedIndex_ = feedIndex;
    goToScreen(Screen::Fetching);
}

// ─── Add feed (renderMenu character picker) ─────────────────────────────────

void RssCore::openAddFeed() {
    urlBuffer_ = "https://";
    urlKeySelected_ = kUrlActionRowCount;  // land on the first character key
    goToScreen(Screen::AddFeed);
}

void RssCore::appendUrlChar(char c) {
    if (urlBuffer_.length() + 1 >= kMaxUrlLen) return;
    urlBuffer_ += c;
}

void RssCore::handleAddFeedTouch(const PluginTouchEvent* event) {
    if (event->phase == 0) return;
    if (event->phase != 2) return;

    const int height = display_ && display_->logicalHeight ? display_->logicalHeight() : 172;
    constexpr uint8_t kTotalKeys = kUrlActionRowCount + kUrlKeyCharCount;
    const uint8_t visibleCount = static_cast<uint8_t>(
        std::min(static_cast<int>(kTotalKeys), std::max(1, height / kMenuRowHeight)));

    const int deltaY = static_cast<int>(event->y) - static_cast<int>(touchStartY_);
    const int absDeltaY = deltaY < 0 ? -deltaY : deltaY;
    constexpr int kSwipeThresholdPx = 30;
    if (absDeltaY >= kSwipeThresholdPx) {
        int next = static_cast<int>(urlKeySelected_) + (deltaY < 0 ? visibleCount : -visibleCount);
        if (next < 0) next = 0;
        if (next >= static_cast<int>(kTotalKeys)) next = kTotalKeys - 1;
        urlKeySelected_ = static_cast<uint8_t>(next);
        return;
    }

    if (event->timestampMs - lastActionMs_ < kActionCooldownMs) return;
    lastActionMs_ = event->timestampMs;

    // Mirrors DisplayManager::renderMenu()'s centered-window scroll math —
    // see DictaphoneCore::handleRenameTouch() for the same derivation.
    size_t firstVisible = 0;
    if (urlKeySelected_ >= visibleCount / 2) {
        firstVisible = urlKeySelected_ - visibleCount / 2;
    }
    if (firstVisible + visibleCount > kTotalKeys) {
        firstVisible = kTotalKeys - visibleCount;
    }
    const int totalHeight = kMenuRowHeight * static_cast<int>(visibleCount);
    const int startY = std::max(0, (height - totalHeight) / 2);
    if (static_cast<int>(event->y) < startY || static_cast<int>(event->y) >= startY + totalHeight) {
        return;
    }
    const size_t tappedRow = static_cast<size_t>((static_cast<int>(event->y) - startY) / kMenuRowHeight);
    const size_t tappedIndex = firstVisible + tappedRow;
    if (tappedIndex >= kTotalKeys) return;

    urlKeySelected_ = static_cast<uint8_t>(tappedIndex);

    if (tappedIndex == 0) {
        // Save
        if (urlBuffer_.length() > 0 && feedCount_ < kRssMaxFeeds) {
            String url = urlBuffer_;
            if (url.indexOf("://") < 0) {
                url = "https://" + url;
            }
            feedUrls_[feedCount_] = url;
            feedCount_++;
            saveFeeds();
        }
        goToScreen(Screen::FeedList);
    } else if (tappedIndex == 1) {
        // Backspace
        if (urlBuffer_.length() > 0) {
            urlBuffer_.remove(urlBuffer_.length() - 1);
        }
    } else if (tappedIndex == 2) {
        // Cancel
        goToScreen(Screen::FeedList);
    } else {
        appendUrlChar(kUrlKeyChars[tappedIndex - kUrlActionRowCount]);
    }
}

// ─── Article list (renderMenu) ──────────────────────────────────────────────

void RssCore::handleArticleListTouch(const PluginTouchEvent* event) {
    if (event->phase != 2) return;
    if (event->timestampMs - lastActionMs_ < kActionCooldownMs) return;
    lastActionMs_ = event->timestampMs;

    const int height = display_ && display_->logicalHeight ? display_->logicalHeight() : 172;
    const uint8_t totalRows = static_cast<uint8_t>(articles_.size() + 1);  // +1 for "< Back"
    const uint8_t visibleCount = static_cast<uint8_t>(
        std::min(static_cast<int>(totalRows), std::max(1, height / kMenuRowHeight)));

    size_t firstVisible = 0;
    if (articleListSelected_ >= visibleCount / 2) {
        firstVisible = articleListSelected_ - visibleCount / 2;
    }
    if (firstVisible + visibleCount > totalRows) {
        firstVisible = totalRows - visibleCount;
    }
    const int totalHeight = kMenuRowHeight * static_cast<int>(visibleCount);
    const int startY = std::max(0, (height - totalHeight) / 2);
    if (static_cast<int>(event->y) < startY || static_cast<int>(event->y) >= startY + totalHeight) {
        return;
    }
    const size_t tappedRow = static_cast<size_t>((static_cast<int>(event->y) - startY) / kMenuRowHeight);
    const size_t tappedIndex = firstVisible + tappedRow;
    if (tappedIndex >= totalRows) return;

    articleListSelected_ = static_cast<uint8_t>(tappedIndex);

    if (tappedIndex == 0) {
        goToScreen(Screen::FeedList);
    } else {
        openArticle(static_cast<uint8_t>(tappedIndex - 1));
    }
}

void RssCore::openArticle(uint8_t index) {
    if (index >= articles_.size()) return;
    openArticleIndex_ = index;
    articleScrollLine_ = 0;
    articleTotalLines_ = 0;
    goToScreen(Screen::ArticleReader);
}

void RssCore::handleArticleReaderTouch(const PluginTouchEvent* event) {
    if (event->phase != 2) return;
    if (event->timestampMs - lastActionMs_ < kActionCooldownMs) return;
    lastActionMs_ = event->timestampMs;

    constexpr uint16_t kHeaderZoneHeight = 24;
    if (event->y < kHeaderZoneHeight) {
        goToScreen(Screen::ArticleList);
        return;
    }

    const int deltaY = static_cast<int>(event->y) - static_cast<int>(touchStartY_);
    const int absDeltaY = deltaY < 0 ? -deltaY : deltaY;
    constexpr int kSwipeThresholdPx = 24;
    const int maxScroll = std::max(0, articleTotalLines_ - 1);

    if (absDeltaY >= kSwipeThresholdPx) {
        int next = articleScrollLine_ + (deltaY < 0 ? kArticleVisibleLinesPerPage
                                                     : -kArticleVisibleLinesPerPage);
        articleScrollLine_ = std::max(0, std::min(next, maxScroll));
        return;
    }

    const int width = display_ && display_->logicalWidth ? display_->logicalWidth() : 640;
    if (event->x < static_cast<uint16_t>(width / 2)) {
        articleScrollLine_ = std::max(0, articleScrollLine_ - kArticleVisibleLinesPerPage);
    } else {
        articleScrollLine_ = std::min(maxScroll, articleScrollLine_ + kArticleVisibleLinesPerPage);
    }
}

// ─── Screen drawing ─────────────────────────────────────────────────────────

void RssCore::draw() {
    switch (screen_) {
        case Screen::Main: drawMain(); break;
        case Screen::FeedList: drawFeedList(); break;
        case Screen::ConfirmDeleteFeed: drawConfirmDeleteFeed(); break;
        case Screen::AddFeed: drawAddFeed(); break;
        case Screen::Fetching: drawFetching(); break;
        case Screen::ArticleList: drawArticleList(); break;
        case Screen::ArticleReader: drawArticleReader(); break;
        case Screen::Error: drawError(); break;
    }
}

void RssCore::drawMain() {
    if (!display_->renderButtonPair) return;
    const int lang = display_->languageIndex ? display_->languageIndex() : 0;

    char leftLabel[32];
    if (feedCount_ > 0) {
        snprintf(leftLabel, sizeof(leftLabel), "%s (%d)", rssText(RssStr::Feeds, lang), feedCount_);
    } else {
        snprintf(leftLabel, sizeof(leftLabel), "%s", rssText(RssStr::Feeds, lang));
    }

    display_->renderButtonPair(leftLabel, PLUGIN_ICON_BOOK, false, rssText(RssStr::AddFeed, lang),
                               PLUGIN_ICON_NONE);
}

void RssCore::drawFeedList() {
    if (!display_->renderDeletableList) return;
    const int lang = display_->languageIndex ? display_->languageIndex() : 0;

    const uint8_t rowCount = static_cast<uint8_t>(feedCount_ + 1);
    const char* items[kFeedListVisibleRows];
    char addLabel[32];
    snprintf(addLabel, sizeof(addLabel), "+ %s", rssText(RssStr::AddFeed, lang));

    uint8_t visibleCount = 0;
    for (uint8_t i = 0; i < kFeedListVisibleRows && i < rowCount; ++i) {
        items[i] = (i == feedCount_) ? addLabel : feedUrls_[i].c_str();
        visibleCount++;
    }

    display_->renderDeletableList(items, visibleCount, feedListSelected_, 0, 1);
}

void RssCore::drawConfirmDeleteFeed() {
    if (!display_->renderButtonPair) return;
    const int lang = display_->languageIndex ? display_->languageIndex() : 0;
    display_->renderButtonPair(rssText(RssStr::Cancel, lang), PLUGIN_ICON_NONE, false,
                               rssText(RssStr::Delete, lang), PLUGIN_ICON_DELETE);
}

void RssCore::drawAddFeed() {
    if (!display_->renderMenu) return;
    const int lang = display_->languageIndex ? display_->languageIndex() : 0;

    char saveLabel[kMaxUrlLen + 24];
    snprintf(saveLabel, sizeof(saveLabel), "%s: %s", rssText(RssStr::Save, lang), urlBuffer_.c_str());

    const char* items[kUrlActionRowCount + kUrlKeyCharCount];
    items[0] = saveLabel;
    items[1] = rssText(RssStr::Backspace, lang);
    items[2] = rssText(RssStr::Cancel, lang);

    char keyLabels[kUrlKeyCharCount][2];
    for (size_t i = 0; i < kUrlKeyCharCount; ++i) {
        keyLabels[i][0] = kUrlKeyChars[i];
        keyLabels[i][1] = '\0';
        items[kUrlActionRowCount + i] = keyLabels[i];
    }

    display_->renderMenu(items, static_cast<uint8_t>(kUrlActionRowCount + kUrlKeyCharCount),
                         urlKeySelected_);
}

void RssCore::drawFetching() {
    if (!display_->renderProgress) return;
    const int lang = display_->languageIndex ? display_->languageIndex() : 0;

    const PluginNetworkStatus status =
        network_ && network_->fetchStatus ? network_->fetchStatus() : PLUGIN_NETWORK_IDLE;
    const char* line2 = (status == PLUGIN_NETWORK_CONNECTING)
                            ? rssText(RssStr::Connecting, lang)
                            : rssText(RssStr::Fetching, lang);

    display_->renderProgress(rssText(RssStr::Rss, lang), feedUrls_[fetchingFeedIndex_].c_str(), line2,
                             -1);
}

void RssCore::drawArticleList() {
    if (!display_->renderMenu) return;
    const int lang = display_->languageIndex ? display_->languageIndex() : 0;

    if (articles_.empty()) {
        if (display_->renderStatus) {
            display_->renderStatus(rssText(RssStr::NoArticles, lang), rssText(RssStr::TapToGoBack, lang),
                                   "");
        }
        return;
    }

    std::vector<String> labels;
    labels.reserve(articles_.size() + 1);
    labels.push_back(String("< ") + (feedTitle_.isEmpty() ? rssText(RssStr::Feeds, lang) : feedTitle_));
    for (const Article& article : articles_) {
        labels.push_back(article.title.isEmpty() ? rssText(RssStr::NoContent, lang) : article.title);
    }

    std::vector<const char*> items;
    items.reserve(labels.size());
    for (const String& label : labels) items.push_back(label.c_str());

    display_->renderMenu(items.data(), static_cast<uint8_t>(items.size()), articleListSelected_);
}

void RssCore::drawArticleReader() {
    if (!display_->renderArticleReader) return;
    if (openArticleIndex_ >= articles_.size()) return;

    const Article& article = articles_[openArticleIndex_];
    const int lang = display_->languageIndex ? display_->languageIndex() : 0;
    const String body = article.body.isEmpty() ? rssText(RssStr::NoContent, lang) : article.body;

    articleTotalLines_ =
        display_->renderArticleReader(article.title.c_str(), body.c_str(), articleScrollLine_);
}

void RssCore::drawError() {
    if (!display_->renderStatus) return;
    const int lang = display_->languageIndex ? display_->languageIndex() : 0;
    display_->renderStatus(rssText(RssStr::ErrorTitle, lang), errorMessage_.c_str(),
                           rssText(RssStr::TapToGoBack, lang));
}

// ─── Feed index persistence ──────────────────────────────────────────────────

bool RssCore::loadFeeds() {
    feedCount_ = 0;
    if (!storage_ || !storage_->readFile) return false;

    uint8_t buf[2048];
    const int bytesRead = storage_->readFile("feeds/index.txt", buf, sizeof(buf) - 1);
    if (bytesRead <= 0) return true;  // No index yet — first run.

    buf[bytesRead] = '\0';
    char* line = strtok(reinterpret_cast<char*>(buf), "\n");
    while (line && feedCount_ < kRssMaxFeeds) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == ' ')) {
            line[--len] = '\0';
        }
        if (len > 0) {
            feedUrls_[feedCount_] = line;
            feedCount_++;
        }
        line = strtok(nullptr, "\n");
    }
    return true;
}

void RssCore::saveFeeds() {
    if (!storage_ || !storage_->writeFile) return;

    String content;
    for (uint8_t i = 0; i < feedCount_; ++i) {
        content += feedUrls_[i];
        content += "\n";
    }
    storage_->writeFile("feeds/index.txt", reinterpret_cast<const uint8_t*>(content.c_str()),
                        content.length());
}

// ─── Plugin SDK VTable glue ──────────────────────────────────────────────────

namespace {

PluginResult rssInit(PluginContext* ctx) {
    s_instance = new RssCore(ctx->display, ctx->storage, ctx->network);
    if (!s_instance) return PLUGIN_ERROR_MEMORY;
    if (!s_instance->begin()) {
        delete s_instance;
        s_instance = nullptr;
        return PLUGIN_ERROR_INIT;
    }
    return PLUGIN_OK;
}

void rssDestroy() {
    if (s_instance) {
        s_instance->shutdown();
        delete s_instance;
        s_instance = nullptr;
    }
}

void rssUpdate(uint32_t nowMs) {
    if (s_instance) s_instance->update(nowMs);
}

void rssHandleButton(const PluginButtonEvent* event) {
    if (s_instance) s_instance->handleButton(event);
}

void rssHandleTouch(const PluginTouchEvent* event) {
    if (s_instance) s_instance->handleTouch(event);
}

void rssDraw() {
    if (s_instance) s_instance->draw();
}

PluginInfo rssGetInfo() {
    return {"RSS", "1.0.0", PLUGIN_SDK_VERSION};
}

}  // namespace

PluginVTable RssPlugin::vtable() {
    return {
        rssInit, rssDestroy, rssUpdate, rssHandleButton, rssHandleTouch, rssDraw, rssGetInfo,
    };
}
