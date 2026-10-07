// ─────────────────────────────────────────────────────────────────────────────
// Smart Building Gateway — application orchestrator
//
// Owns the single non-blocking tick and decides the order the modules run in.
// ─────────────────────────────────────────────────────────────────────────────

#pragma once

#include <Arduino.h>

namespace sys {

/// Bring the whole system up.
void setup();

/// One pass of the control cycle. Called from loop() as fast as possible;
/// it returns immediately unless TICK_MS have elapsed.
void loop(unsigned long now);

/// How often the full status is echoed to Serial, independent of changes.
constexpr unsigned long STATUS_INTERVAL_MS = 5000UL;

void reportStatus(unsigned long now);

} // namespace app