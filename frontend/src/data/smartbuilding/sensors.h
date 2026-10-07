// ─────────────────────────────────────────────────────────────────────────────
// Smart Building Gateway — sensor processing
//
// Samples the environment on its own schedule and records how fresh each
// reading is. Nothing here makes a decision; it only fills in AppState.
// ─────────────────────────────────────────────────────────────────────────────

#pragma once

#include <Arduino.h>

namespace sensors {

/// Call every base tick. Reads whatever is due and stamps the reading times.
void tick(unsigned long now);

/// Force the next tick to sample the DHT22 immediately. Used once at startup
/// so the LCD has real numbers straight away instead of after 2s of dashes.
void requestImmediateClimate();

/// The ADC counts the configured light threshold corresponds to. Exposed so
/// the display and the serial report can talk in the same units.
int lightThresholdAdc();

} // namespace sensors