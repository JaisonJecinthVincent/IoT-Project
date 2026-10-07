// ─────────────────────────────────────────────────────────────────────────────
// Smart Building Gateway — application orchestrator
//
// Owns the single non-blocking tick and decides the order the modules run in.
// ─────────────────────────────────────────────────────────────────────────────

#include "app.h"

#include "config.h"
#include "display.h"
#include "hal.h"
#include "logic.h"
#include "sensors.h"
#include "state.h"

namespace sys {
namespace {

unsigned long lastTickAt = 0;
unsigned long lastStatusAt = 0;

} // namespace

void setup() {
  stateInit();
  hal::init();
  display::init();
  // Give the first LCD paint something real to show.
  sensors::requestImmediateClimate();
  lastTickAt = millis();
  lastStatusAt = millis();
  Serial.println(F("[sys] control loop starting"));
}

void loop(unsigned long now) {
  if ((now - lastTickAt) < cfg::TICK_MS) {
    return;
  }
  lastTickAt = now;

  // Fixed order. Buttons first so an operator action is reflected in the same
  // tick's decisions; actuators last so every rule has finished before
  // hardware is touched.
  hal::service(now);
  logic::handleButtons(now);
  sensors::tick(now);
  logic::updateOccupancy(now);
  logic::updateLighting(now);
  logic::updateFan(now);
  logic::updateSecurity(now);
  logic::updateDoor(now);
  logic::applyActuators();
  display::tick(now);
  logic::reportChanges(now);

  if ((now - lastStatusAt) >= STATUS_INTERVAL_MS) {
    lastStatusAt = now;
    reportStatus(now);
  }
}

void reportStatus(unsigned long now) {
  (void)now;
  Serial.print(F("[status] "));
  if (app.climateValid) {
    Serial.print(app.temperature, 1);
    Serial.print(F("C "));
    Serial.print(app.humidity, 1);
    Serial.print(F("% "));
  } else {
    Serial.print(F("-- "));
  }
  Serial.print(F("light="));
  Serial.print(app.lightLevel);
  Serial.print(F(" motion="));
  Serial.print(app.motionDetected ? 1 : 0);
  Serial.print(F(" occ="));
  Serial.print(app.occupied ? 1 : 0);
  Serial.print(F(" lightOn="));
  Serial.print(app.lightOn ? 1 : 0);
  Serial.print(F(" fanOn="));
  Serial.print(app.fanOn ? 1 : 0);
  Serial.print(F(" armed="));
  Serial.print(app.securityArmed ? 1 : 0);
  Serial.print(F(" alarm="));
  Serial.print(app.alarmActive ? 1 : 0);
  Serial.print(F(" door="));
  Serial.println(app.doorLocked ? F("locked") : F("unlocked"));
}

} // namespace sys