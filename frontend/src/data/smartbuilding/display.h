// ─────────────────────────────────────────────────────────────────────────────
// Smart Building Gateway — local display
//
// Four status pages on the 16x2 panel, rotating every 3 seconds. The driver
// keeps a shadow of what is on the glass and only pushes a row when its
// content actually changed, so the LCD bus stays quiet while the numbers
// settle. That matters here: the simulator rewrites a changed row either way,
// and a redraw on every 20ms tick would swamp the serial output.
//
// A byte value of 0x01 in a row buffer is the custom degree glyph; every
// other byte is plain ASCII.
// ─────────────────────────────────────────────────────────────────────────────

#pragma once

#include <Arduino.h>

namespace display {

// CGRAM slot holding the degree sign. Slot 0 is deliberately avoided: writing
// a NUL into a row makes the HD44780 decoder render it as a blank instead of
// the custom glyph.
constexpr uint8_t DEG = 0x01;

void init();

/// Advance page rotation and repaint if needed.
void tick(unsigned long now);

} // namespace display