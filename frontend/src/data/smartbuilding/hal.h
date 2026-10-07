// ─────────────────────────────────────────────────────────────────────────────
// Smart Building Gateway — hardware abstraction layer
//
// The only module that knows a GPIO number, an ADC channel or that a servo
// takes degrees. Everything above it works in terms of "the light" or "the
// door", which is what keeps the rule engine free of hardware details.
// ─────────────────────────────────────────────────────────────────────────────

#pragma once

#include <Arduino.h>

namespace hal {

/// Configure every pin and attach the servo and LCD. Call once from setup().
void init();

// ── Sensors ────────────────────────────────────────────────────────────────

/// Raw ADC counts from the photoresistor on A0 (0..1023).
int readLightLevel();

/// Current PIR output level.
bool readMotion();

/// Sample the DHT22.
/// @param t receives deg C, NaN on failure
/// @param h receives %, NaN on failure
/// @return true when the reading was valid
bool readClimate(float &t, float &h);

// ── Buttons ────────────────────────────────────────────────────────────────

enum class Button { Light, Security, Door };

/// True exactly once per press. Consumes the edge; call from the base tick.
bool buttonPressed(Button b);

// ── Actuators ──────────────────────────────────────────────────────────────

void setRoomLight(bool on);
void setFanIndicator(bool on);
void setSecurityIndicator(bool on);

/// Drive the alarm buzzer. Blinks between two pitches rather than holding a
/// single tone, so a working tone() is distinguishable from a dead pin.
void setAlarm(bool on);

/// Move the door lock servo toward its locked or unlocked position.
void setDoorLocked(bool locked);

/// Per-tick housekeeping: button debounce, servo easing and the alarm
/// cadence. Must be called from the base tick or nothing moves smoothly.
void service(unsigned long now);

// ── Display ────────────────────────────────────────────────────────────────
// The LiquidCrystal object stays private to this module so no other layer can
// scribble on the screen behind the display driver's back.

/// Bring up the 16x2 panel and register the custom glyphs.
void lcdInit();

/// Clear the panel and both rows of the driver's shadow buffer.
void lcdClear();

/// Write `text` at a row, overwriting the rest of that row with spaces so no
/// stale characters survive a shorter message.
void lcdWriteRow(uint8_t row, const char *text);

/// Register an 8-byte glyph in CGRAM slot `index` (0..7).
void lcdCreateChar(uint8_t index, const uint8_t bitmap[8]);

} // namespace hal