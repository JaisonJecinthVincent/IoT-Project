// ─────────────────────────────────────────────────────────────────────────────
// Smart Building Gateway
// Arduino Mega 2560 — free, fully local edge controller.
//
// Everything runs in one non-blocking tick; no module uses delay() for
// anything except the hardware settle times the datasheets demand. The pin
// map and every tunable are in config.h.
// ─────────────────────────────────────────────────────────────────────────────

#include "app.h"
#include "config.h"

void setup() { sys::setup(); }

void loop() { sys::loop(millis()); }