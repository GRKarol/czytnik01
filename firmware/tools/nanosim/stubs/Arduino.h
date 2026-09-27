// Host-side stand-in for the bits of the Arduino core the display code uses.
// Only for tools/nanosim -- never compiled into firmware.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#define PROGMEM
#define IRAM_ATTR
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#define pgm_read_word(addr) (*(const uint16_t *)(addr))

uint32_t millis();
uint32_t micros();
void delay(uint32_t ms);
inline void yield() {}

class String {
 public:
  String() = default;
  String(const char *s) : s_(s ? s : "") {}
  String(const std::string &s) : s_(s) {}
  String(char c) : s_(1, c) {}
  String(int v) : s_(std::to_string(v)) {}
  String(unsigned v) : s_(std::to_string(v)) {}
  String(long v) : s_(std::to_string(v)) {}
  String(unsigned long v) : s_(std::to_string(v)) {}
  String(long long v) : s_(std::to_string(v)) {}
  String(unsigned long long v) : s_(std::to_string(v)) {}
  String(float v, int d = 2) { char b[32]; snprintf(b, sizeof b, "%.*f", d, v); s_ = b; }
  String(double v, int d = 2) { char b[32]; snprintf(b, sizeof b, "%.*f", d, v); s_ = b; }
  unsigned length() const { return static_cast<unsigned>(s_.size()); }
  bool isEmpty() const { return s_.empty(); }
  const char *c_str() const { return s_.c_str(); }
  char operator[](unsigned i) const { return i < s_.size() ? s_[i] : 0; }
  char &operator[](unsigned i) { return s_[i]; }
  char charAt(unsigned i) const { return (*this)[i]; }
  void setCharAt(unsigned i, char c) { if (i < s_.size()) s_[i] = c; }
  String substring(unsigned from) const { return from >= s_.size() ? String() : String(s_.substr(from)); }
  String substring(unsigned from, unsigned to) const {
    if (from > to) std::swap(from, to);
    if (from >= s_.size()) return String();
    return String(s_.substr(from, std::min<size_t>(to, s_.size()) - from));
  }
  int indexOf(char c, unsigned from = 0) const { auto p = s_.find(c, from); return p == std::string::npos ? -1 : (int)p; }
  int indexOf(const String &t, unsigned from = 0) const { auto p = s_.find(t.s_, from); return p == std::string::npos ? -1 : (int)p; }
  int indexOf(const char *t, unsigned from = 0) const { return indexOf(String(t), from); }
  int lastIndexOf(char c) const { auto p = s_.rfind(c); return p == std::string::npos ? -1 : (int)p; }
  int lastIndexOf(char c, int from) const { if (from < 0) return -1; auto p = s_.rfind(c, from); return p == std::string::npos ? -1 : (int)p; }
  int lastIndexOf(const String &t) const { auto p = s_.rfind(t.s_); return p == std::string::npos ? -1 : (int)p; }
  int lastIndexOf(const char *t) const { return lastIndexOf(String(t)); }
  bool startsWith(const String &p) const { return s_.compare(0, p.s_.size(), p.s_) == 0; }
  bool endsWith(const String &p) const { return s_.size() >= p.s_.size() && s_.compare(s_.size() - p.s_.size(), p.s_.size(), p.s_) == 0; }
  void trim() {
    size_t a = 0, b = s_.size();
    while (a < b && isspace((unsigned char)s_[a])) ++a;
    while (b > a && isspace((unsigned char)s_[b - 1])) --b;
    s_ = s_.substr(a, b - a);
  }
  void toLowerCase() { for (auto &c : s_) c = (char)tolower((unsigned char)c); }
  void toUpperCase() { for (auto &c : s_) c = (char)toupper((unsigned char)c); }
  void remove(unsigned i) { if (i < s_.size()) s_.erase(i); }
  void remove(unsigned i, unsigned n) { if (i < s_.size()) s_.erase(i, n); }
  void reserve(unsigned n) { s_.reserve(n); }
  void replace(const String &a, const String &b) {
    if (a.s_.empty()) return;
    size_t p = 0;
    while ((p = s_.find(a.s_, p)) != std::string::npos) { s_.replace(p, a.s_.size(), b.s_); p += b.s_.size(); }
  }
  long toInt() const { return atol(s_.c_str()); }
  float toFloat() const { return (float)atof(s_.c_str()); }
  bool equals(const String &o) const { return s_ == o.s_; }
  bool equalsIgnoreCase(const String &o) const { return strcasecmp(s_.c_str(), o.s_.c_str()) == 0; }
  bool concat(const String &o) { s_ += o.s_; return true; }
  String &operator+=(const String &o) { s_ += o.s_; return *this; }
  String &operator+=(const char *o) { s_ += o ? o : ""; return *this; }
  String &operator+=(char c) { s_ += c; return *this; }
  String &operator+=(int v) { s_ += std::to_string(v); return *this; }
  String &operator+=(unsigned v) { s_ += std::to_string(v); return *this; }
  bool operator==(const String &o) const { return s_ == o.s_; }
  bool operator!=(const String &o) const { return s_ != o.s_; }
  bool operator==(const char *o) const { return s_ == (o ? o : ""); }
  bool operator!=(const char *o) const { return !(*this == o); }
  bool operator<(const String &o) const { return s_ < o.s_; }
  friend String operator+(const String &a, const String &b) { return String(a.s_ + b.s_); }
  friend String operator+(const String &a, const char *b) { return String(a.s_ + (b ? b : "")); }
  friend String operator+(const char *a, const String &b) { return String(std::string(a ? a : "") + b.s_); }
  friend String operator+(const String &a, char b) { return String(a.s_ + b); }
  const std::string &str() const { return s_; }

 private:
  std::string s_;
};

struct SerialStub {
  void begin(unsigned long) {}
  int printf(const char *fmt, ...) { va_list ap; va_start(ap, fmt); int n = vfprintf(stderr, fmt, ap); va_end(ap); return n; }
  void println(const String &s = String()) { fprintf(stderr, "%s\n", s.c_str()); }
  void println(const char *s) { fprintf(stderr, "%s\n", s); }
  void print(const String &s) { fprintf(stderr, "%s", s.c_str()); }
  void print(const char *s) { fprintf(stderr, "%s", s); }
};
extern SerialStub Serial;
