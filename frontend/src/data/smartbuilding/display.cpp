#include "display.h"

#include "config.h"
#include "hal.h"
#include "sensors.h"
#include "state.h"

#include <math.h>

namespace display {
namespace {

constexpr uint8_t COLS = 16;
constexpr uint8_t PAGE_COUNT = 4;

// 5x8 bitmap for the custom degree glyph.
const uint8_t GLYPH_DEGREE[8] = {0x06, 0x09, 0x09, 0x06,
                                  0x00, 0x00, 0x00, 0x00};

unsigned long lastPageAt = 0;
unsigned long lastDrawAt = 0;

/// Rows currently on the glass, kept by hal::lcdWriteRow.
char shadow[2][COLS + 1] = {{0}, {0}};

/// Copy `in` into `out`, right-aligned in `width`, padded with spaces.
void fit(char *out, size_t width, const char *in) {
  const size_t len = strlen(in);
  const size_t pad = (len < width) ? (width - len) : 0;
  memset(out, ' ', width);
  out[width] = '\0';
  if (len > 0) {
    memcpy(out + pad, in, (len < width) ? len : width);
  }
}

/// Write a row only when it differs from the shadow, and only if the last
/// repaint was long enough ago.
void putRow(uint8_t row, const char *text, unsigned long now) {
  if (strcmp(shadow[row], text) == 0) {
    return;
  }
  if ((now - lastDrawAt) < cfg::LCD_MIN_REDRAW_MS) {
    return;
  }
  hal::lcdWriteRow(row, text);
  memcpy(shadow[row], text, COLS + 1);
  lastDrawAt = now;
}

/// Render a value as "-12.3" using integer conversions only.
///
/// avr-libc's printf has no floating-point support, so a "%.1f" conversion
/// prints garbage on AVR (it needs -lprintf_flt, which costs ~1.5 kB of flash
/// for output that a display buffer does not warrant). Scaling to tenths and
/// formatting the result as a long plus a digit is exact for the -40..+80
/// degC / 0..100% range a DHT22 reports.
size_t fmtFixed1(char *out, size_t cap, float value) {
  if (isnan(value)) {
    return static_cast<size_t>(snprintf(out, cap, "--"));
  }
  long scaled = lround(value * 10.0);
  const bool negative = scaled < 0;
  if (negative) {
    scaled = -scaled;
  }
  return static_cast<size_t>(
      snprintf(out, cap, "%s%ld.%d", negative ? "-" : "", scaled / 10,
               static_cast<int>(scaled % 10)));
}

/// A readable substitute for a reading that is not available right now.
void fmtClimate(char *out, size_t width, float value, bool valid,
                const char *suffix) {
  char number[12];
  char buf[COLS + 1];
  if (valid) {
    fmtFixed1(number, sizeof(number), value);
  } else {
    snprintf(number, sizeof(number), "--");
  }
  snprintf(buf, sizeof(buf), "%s%s", number, suffix);
  fit(out, width, buf);
}

void pageClimate(char *top, char *bottom) {
  char temperature[12];
  char humidity[12];

  fmtFixed1(temperature, sizeof(temperature), app.temperature);
  fmtClimate(humidity, 8, app.humidity, app.climateValid, "%");

  // "Temp:  " + value + degree glyph + "C"  ==  7 + 6 + 1 + 1 = 15 columns.
  constexpr size_t VALUE_COLS = 6;
  memcpy(top, "Temp:  ", 7);
  fit(top + 7, VALUE_COLS, temperature);
  top[7 + VALUE_COLS] = display::DEG;
  top[7 + VALUE_COLS + 1] = 'C';

  snprintf(bottom, COLS + 1, "Hum  : %s", humidity);
}

void pageLighting(char *top, char *bottom) {
  char level[COLS + 1];
  if (app.lightLevel >= 1023) {
    snprintf(level, sizeof(level), "100%%");
  } else {
    snprintf(level, sizeof(level), "%lu%%",
             (unsigned long)((app.lightLevel * 100UL) / 1024UL));
  }
  snprintf(top, COLS + 1, "Light: %s", level);
  snprintf(bottom, COLS + 1, "%s  occ:%s",
           app.occupied ? "LIGHT ON " : "LIGHT OFF",
           app.occupied ? "YES" : "NO ");
}

void pageSecurity(char *top, char *bottom) {
  snprintf(top, COLS + 1, "Sec : %s", app.securityArmed ? "ARMED" : "OFF");
  snprintf(bottom, COLS + 1, "Alarm:%s",
           app.alarmActive ? "ACTIVE" : "none  ");
}

void pageDoorFan(char *top, char *bottom) {
  snprintf(top, COLS + 1, "Door: %s",
           app.doorLocked ? "LOCKED" : "UNLOCKED");
  snprintf(bottom, COLS + 1, "Fan : %s%s", app.fanOn ? "ON " : "OFF",
           app.fanOn ? " " : " ");
}

void buildPage(uint8_t page, char *top, char *bottom) {
  memset(top, ' ', COLS);
  memset(bottom, ' ', COLS);
  top[COLS] = '\0';
  bottom[COLS] = '\0';

  switch (page) {
  case 0:
    pageClimate(top, bottom);
    break;
  case 1:
    pageLighting(top, bottom);
    break;
  case 2:
    pageSecurity(top, bottom);
    break;
  default:
    pageDoorFan(top, bottom);
    break;
  }
}

} // namespace

void init() {
  hal::lcdInit();
  hal::lcdCreateChar(DEG, GLYPH_DEGREE);
  hal::lcdClear();
  memset(shadow, 0, sizeof(shadow));
  lastDrawAt = 0;
  lastPageAt = 0;
  app.lcdPage = 0;
}

void tick(unsigned long now) {
  if ((now - lastPageAt) >= cfg::LCD_PAGE_INTERVAL_MS) {
    lastPageAt = now;
    app.lcdPage = (app.lcdPage + 1) % PAGE_COUNT;
  }

  char top[COLS + 1];
  char bottom[COLS + 1];
  buildPage(app.lcdPage, top, bottom);
  putRow(0, top, now);
  putRow(1, bottom, now);
}

} // namespace display