#include "storage/PresetManager.h"

#include <SD_MMC.h>
#include <algorithm>

// --- Captured Settings Key Table ---

struct CapturedKey {
  const char* key;
  enum Type : uint8_t { U8, U16, I8, Bool } type;
  int minVal;
  int maxVal;
  int defaultVal;
};

static constexpr CapturedKey kCapturedKeys[] = {
    {"wpm", CapturedKey::U16, 10, 1000, 300},
    {"bright", CapturedKey::U8, 0, 4, 3},
    {"dark", CapturedKey::Bool, 0, 1, 1},
    {"night", CapturedKey::Bool, 0, 1, 0},
    {"read_mode", CapturedKey::U8, 0, 1, 0},
    {"handed", CapturedKey::U8, 0, 1, 0},
    {"phantom_on", CapturedKey::Bool, 0, 1, 1},
    {"prog_md", CapturedKey::U8, 0, 2, 0},
    {"bat_md", CapturedKey::U8, 0, 2, 0},
    {"read_bat", CapturedKey::Bool, 0, 1, 1},
    {"read_ch", CapturedKey::Bool, 0, 1, 0},
    {"read_pct", CapturedKey::Bool, 0, 1, 0},
    {"font_size", CapturedKey::U8, 0, 2, 0},
    {"typeface", CapturedKey::U8, 0, 2, 0},
    {"type_hlt", CapturedKey::Bool, 0, 1, 1},
    {"pace_lms", CapturedKey::U16, 0, 600, 200},
    {"pace_cms", CapturedKey::U16, 0, 600, 200},
    {"pace_pms", CapturedKey::U16, 0, 600, 200},
    {"pause_md", CapturedKey::U8, 0, 1, 0},
    {"type_trk", CapturedKey::I8, -2, 3, 0},
    {"type_anc", CapturedKey::U8, 30, 40, 30},
    {"type_wid", CapturedKey::U8, 12, 30, 30},
    {"type_gap", CapturedKey::U8, 2, 8, 5},
    {"sc_font", CapturedKey::U8, 0, 8, 4},
    {"sc_line_sp", CapturedKey::U8, 0, 2, 1},
    {"sc_margin", CapturedKey::U8, 0, 2, 1},
    {"ss_mode", CapturedKey::U8, 0, 6, 0},
    {"ss_timeout", CapturedKey::U8, 0, 5, 2},
    {"focus_clr", CapturedKey::U8, 0, 5, 0},
};

static constexpr size_t kCapturedKeyCount =
    sizeof(kCapturedKeys) / sizeof(kCapturedKeys[0]);

// --- Excluded Keys (never captured in presets) ---

static constexpr const char* kExcludedKeys[] = {
    "wifi_ssid",
    "wifi_pass",
};

// --- Public Methods (stubs) ---

std::vector<PresetManager::PresetInfo> PresetManager::listPresets() {
  std::vector<PresetInfo> results;

  File dir = SD_MMC.open(kPresetsDir);
  if (!dir || !dir.isDirectory()) {
    return results;
  }

  File entry;
  while ((entry = dir.openNextFile())) {
    String filename = entry.name();

    // Only process .json files
    if (!filename.endsWith(".json")) {
      entry.close();
      continue;
    }

    // Read file content
    String content = entry.readString();
    entry.close();

    // Extract the preset name from JSON
    String name = extractPresetName(content);
    if (name.isEmpty()) {
      continue;
    }

    results.push_back(PresetInfo{name, filename});
  }

  dir.close();

  // Sort alphabetically by name
  std::sort(results.begin(), results.end(),
            [](const PresetInfo& a, const PresetInfo& b) {
              return a.name < b.name;
            });

  return results;
}

PresetManager::SaveResult PresetManager::savePreset(const String& name, Preferences& prefs) {
  if (presetCount() >= kMaxPresets) return SaveResult::LimitReached;

  if (!SD_MMC.exists(kPresetsDir)) {
    if (!SD_MMC.mkdir(kPresetsDir)) return SaveResult::DirectoryError;
  }

  String json = serializePreset(name, prefs);
  String filename = generateFilename();
  String path = String(kPresetsDir) + "/" + filename;

  File file = SD_MMC.open(path, FILE_WRITE);
  if (!file) return SaveResult::WriteError;

  size_t written = file.print(json);
  file.close();

  if (written != json.length()) {
    SD_MMC.remove(path.c_str());
    return SaveResult::WriteError;
  }
  return SaveResult::Ok;
}

PresetManager::RestoreResult PresetManager::restorePreset(const String& filename,
                                                          Preferences& prefs) {
  String path = String(kPresetsDir) + "/" + filename;
  File file = SD_MMC.open(path, FILE_READ);
  if (!file) return RestoreResult::FileNotFound;

  String content = file.readString();
  file.close();

  if (content.isEmpty()) return RestoreResult::ParseError;
  if (!deserializeAndApply(content, prefs)) return RestoreResult::ParseError;

  return RestoreResult::Ok;
}

PresetManager::DeleteResult PresetManager::deletePreset(const String& filename) {
  String path = String(kPresetsDir) + "/" + filename;
  if (!SD_MMC.exists(path.c_str())) return DeleteResult::FileNotFound;
  if (!SD_MMC.remove(path.c_str())) return DeleteResult::SdCardError;
  return DeleteResult::Ok;
}

String PresetManager::validateName(const String& raw) {
  String result;
  result.reserve(kMaxPresetNameLength);

  for (size_t i = 0; i < raw.length() && result.length() < kMaxPresetNameLength; ++i) {
    char c = raw[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_') {
      result += c;
    }
  }
  result.trim();
  return result;
}

size_t PresetManager::presetCount() {
  return listPresets().size();
}

// --- Private Helpers (stubs) ---

String PresetManager::generateFilename() {
  uint32_t ts = millis();
  uint16_t rnd = esp_random() & 0xFFFF;
  char buf[24];
  snprintf(buf, sizeof(buf), "%lu_%04x.json", (unsigned long)ts, rnd);
  return String(buf);
}

String PresetManager::serializePreset(const String& name, Preferences& prefs) {
  String json;
  json.reserve(512);

  json += "{\"name\":\"";
  json += jsonEscape(name);
  json += "\",\"settings\":{";

  for (size_t i = 0; i < kCapturedKeyCount; ++i) {
    const CapturedKey& ck = kCapturedKeys[i];

    if (i > 0) {
      json += ",";
    }

    json += "\"";
    json += ck.key;
    json += "\":";

    switch (ck.type) {
      case CapturedKey::U8: {
        uint8_t val = prefs.getUChar(ck.key, (uint8_t)ck.defaultVal);
        json += String(val);
        break;
      }
      case CapturedKey::U16: {
        uint16_t val = prefs.getUShort(ck.key, (uint16_t)ck.defaultVal);
        json += String(val);
        break;
      }
      case CapturedKey::I8: {
        int8_t val = prefs.getChar(ck.key, (int8_t)ck.defaultVal);
        json += String(val);
        break;
      }
      case CapturedKey::Bool: {
        bool val = prefs.getBool(ck.key, (bool)ck.defaultVal);
        json += val ? "true" : "false";
        break;
      }
    }
  }

  json += "}}";
  return json;
}

bool PresetManager::deserializeAndApply(const String& json, Preferences& prefs) {
  // Find the "settings" key
  int settingsKeyIdx = json.indexOf("\"settings\"");
  if (settingsKeyIdx < 0) return false;

  // Find the colon after "settings"
  int colonIdx = json.indexOf(':', settingsKeyIdx + 10);
  if (colonIdx < 0) return false;

  // Find the opening brace of the settings object
  int braceIdx = -1;
  for (int i = colonIdx + 1; i < (int)json.length(); ++i) {
    char c = json[i];
    if (isspace((unsigned char)c)) continue;
    if (c == '{') {
      braceIdx = i;
      break;
    }
    return false;  // unexpected character before '{'
  }
  if (braceIdx < 0) return false;

  // Scan key-value pairs inside the settings object
  int pos = braceIdx + 1;
  int len = (int)json.length();

  while (pos < len) {
    // Skip whitespace
    while (pos < len && isspace((unsigned char)json[pos])) ++pos;
    if (pos >= len) return false;

    // Check for end of object
    if (json[pos] == '}') break;

    // Skip comma between entries
    if (json[pos] == ',') {
      ++pos;
      continue;
    }

    // Expect opening quote for key
    if (json[pos] != '"') return false;
    ++pos;

    // Extract key name
    String key;
    while (pos < len && json[pos] != '"') {
      key += json[pos];
      ++pos;
    }
    if (pos >= len) return false;
    ++pos;  // skip closing quote

    // Skip whitespace and colon
    while (pos < len && isspace((unsigned char)json[pos])) ++pos;
    if (pos >= len || json[pos] != ':') return false;
    ++pos;  // skip colon
    while (pos < len && isspace((unsigned char)json[pos])) ++pos;
    if (pos >= len) return false;

    // Parse value (number, true, or false)
    int value = 0;

    if (json[pos] == 't') {
      // Check for "true"
      if (pos + 4 <= len && json.substring(pos, pos + 4) == "true") {
        value = 1;
        pos += 4;
      } else {
        return false;
      }
    } else if (json[pos] == 'f') {
      // Check for "false"
      if (pos + 5 <= len && json.substring(pos, pos + 5) == "false") {
        value = 0;
        pos += 5;
      } else {
        return false;
      }
    } else if (json[pos] == '-' || isdigit((unsigned char)json[pos])) {
      // Parse number
      bool negative = false;
      if (json[pos] == '-') {
        negative = true;
        ++pos;
      }
      if (pos >= len || !isdigit((unsigned char)json[pos])) return false;
      int num = 0;
      while (pos < len && isdigit((unsigned char)json[pos])) {
        num = num * 10 + (json[pos] - '0');
        ++pos;
      }
      value = negative ? -num : num;
    } else {
      return false;  // unexpected value type
    }

    // Look up key in kCapturedKeys[]
    const CapturedKey* found = nullptr;
    for (size_t i = 0; i < kCapturedKeyCount; ++i) {
      if (key == kCapturedKeys[i].key) {
        found = &kCapturedKeys[i];
        break;
      }
    }

    // Skip unknown keys (from future firmware versions)
    if (!found) continue;

    // Clamp and write to NVS using appropriate typed putter
    int clamped = clampValue(value, found->minVal, found->maxVal);

    switch (found->type) {
      case CapturedKey::U8:
        prefs.putUChar(found->key, (uint8_t)clamped);
        break;
      case CapturedKey::U16:
        prefs.putUShort(found->key, (uint16_t)clamped);
        break;
      case CapturedKey::I8:
        prefs.putChar(found->key, (int8_t)clamped);
        break;
      case CapturedKey::Bool:
        prefs.putBool(found->key, value != 0);
        break;
    }
  }

  return true;
}

String PresetManager::extractPresetName(const String& json) {
  // Find the "name" key
  int keyIdx = json.indexOf("\"name\"");
  if (keyIdx < 0) return "";

  // Find the colon after "name"
  int colonIdx = json.indexOf(':', keyIdx + 6);
  if (colonIdx < 0) return "";

  // Skip whitespace after colon
  int pos = colonIdx + 1;
  int len = (int)json.length();
  while (pos < len && isspace((unsigned char)json[pos])) ++pos;

  // Expect opening quote for value
  if (pos >= len || json[pos] != '"') return "";
  ++pos;

  // Extract the string value
  String name;
  while (pos < len && json[pos] != '"') {
    if (json[pos] == '\\' && pos + 1 < len) {
      // Handle escape sequences
      ++pos;
      switch (json[pos]) {
        case '"': name += '"'; break;
        case '\\': name += '\\'; break;
        case 'n': name += '\n'; break;
        case 'r': name += '\r'; break;
        case 't': name += '\t'; break;
        default: name += json[pos]; break;
      }
    } else {
      name += json[pos];
    }
    ++pos;
  }

  // Check that we found the closing quote
  if (pos >= len) return "";

  return name;
}

int PresetManager::clampValue(int value, int min, int max) {
  if (value < min) return min;
  if (value > max) return max;
  return value;
}

String PresetManager::jsonEscape(const String& value) {
  String result;
  result.reserve(value.length() + 4);
  for (size_t i = 0; i < value.length(); ++i) {
    char c = value[i];
    switch (c) {
      case '\\': result += "\\\\"; break;
      case '"':  result += "\\\""; break;
      case '\n': result += "\\n"; break;
      case '\r': result += "\\r"; break;
      case '\t': result += "\\t"; break;
      default:   result += c; break;
    }
  }
  return result;
}
