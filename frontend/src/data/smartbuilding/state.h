// ─────────────────────────────────────────────────────────────────────────────
// Smart Building Gateway — central application state
//
// Every signal the application reasons about lives in one struct. No module
// keeps a private copy of a fact another module also owns, so there is no way
// for the rule engine and the display to disagree about the room.
// ─────────────────────────────────────────────────────────────────────────────

#pragma once

#include <Arduino.h>

/// What the operator last asked the lighting rule to do.
enum class Override : uint8_t {
  AUTO, ///< follow the automatic lighting rule
  ON,   ///< operator forced the light on
  OFF,  ///< operator forced the light off
};

struct AppState {
  // ── Sensor readings ──────────────────────────────────────────────────────
  float temperature;   ///< deg C, NaN when the last DHT22 read failed
  float humidity;      ///< %,NaN when the last DHT22 read failed
  int lightLevel;      ///< raw ADC counts from the LDR
  bool motionDetected; ///< the PIR output level sampled this tick

  // ── Derived state ────────────────────────────────────────────────────────
  bool occupied;         ///< motion seen within cfg::OCCUPANCY_TIMEOUT_MS
  bool motionSeen;       ///< false until the PIR has reported motion once
  unsigned long lastMotionAt;

  // ── Actuator state, owned by the rule engine ─────────────────────────────
  bool lightOn;
  bool fanOn;
  bool securityArmed;
  bool alarmActive;
  bool doorLocked;

  // ── Manual override ──────────────────────────────────────────────────────
  Override lightOverride;
  unsigned long overrideUntil;

  // ── Door ─────────────────────────────────────────────────────────────────
  unsigned long doorUnlockedAt; ///< when the lock was released, 0 while locked

  // ── Data freshness ───────────────────────────────────────────────────────
  // A failed read leaves the previous value in place but clears its valid
  // flag, so the rule engine can fall back instead of acting on stale data.
  bool climateValid;
  unsigned long lastClimateRead;

  // ── Display ──────────────────────────────────────────────────────────────
  uint8_t lcdPage;

  // ── Diagnostics ──────────────────────────────────────────────────────────
  bool stateChanged; ///< set when a rule fired, cleared by the reporter
  unsigned long lastAlarmAt;
};

/// The one and only application state.
extern AppState app;

/// Bring every field to a defined value. Called once from setup().
void stateInit();