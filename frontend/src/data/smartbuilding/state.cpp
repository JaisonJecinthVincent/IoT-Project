#include "state.h"

#include <math.h>

/// The one and only application state. Every module reads and writes this
/// object, which is what keeps the rules and the display from disagreeing.
AppState app;

void stateInit() {
  app.temperature = NAN;
  app.humidity = NAN;
  app.lightLevel = 0;
  app.motionDetected = false;

  app.occupied = false;
  app.motionSeen = false;
  app.lastMotionAt = 0;

  app.lightOn = false;
  app.fanOn = false;
  app.securityArmed = false;
  app.alarmActive = false;
  app.doorLocked = true;

  app.lightOverride = Override::AUTO;
  app.overrideUntil = 0;
  app.doorUnlockedAt = 0;

  app.climateValid = false;
  app.lastClimateRead = 0;

  app.lcdPage = 0;
  app.stateChanged = false;
  app.lastAlarmAt = 0;
}