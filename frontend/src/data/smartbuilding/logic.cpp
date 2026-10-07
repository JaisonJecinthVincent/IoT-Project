#include "logic.h"

#include "config.h"
#include "hal.h"
#include "sensors.h"
#include "state.h"

namespace logic {
namespace {

const char *overrideName(Override o) {
  switch (o) {
  case Override::ON:
    return "ON";
  case Override::OFF:
    return "OFF";
  default:
    return "AUTO";
  }
}

/// Set an actuator field and flag the change for the serial report.
void setActuator(bool &field, bool value) {
  if (field == value) {
    return;
  }
  field = value;
  app.stateChanged = true;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
void handleButtons(unsigned long now) {
  // Button 1 — cycle the lighting override AUTO -> ON -> OFF -> AUTO.
  if (hal::buttonPressed(hal::Button::Light)) {
    switch (app.lightOverride) {
    case Override::AUTO:
      app.lightOverride = Override::ON;
      break;
    case Override::ON:
      app.lightOverride = Override::OFF;
      break;
    default:
      app.lightOverride = Override::AUTO;
      break;
    }
    app.overrideUntil = now + cfg::OVERRIDE_HOLD_MS;
    Serial.print(F("[btn] light override -> "));
    Serial.println(overrideName(app.lightOverride));
  }

  // Button 2 — arm or disarm the security system. Disarming also silences
  // any alarm that is currently sounding.
  if (hal::buttonPressed(hal::Button::Security)) {
    setActuator(app.securityArmed, !app.securityArmed);
    if (!app.securityArmed && app.alarmActive) {
      setActuator(app.alarmActive, false);
    }
    Serial.print(F("[btn] security "));
    Serial.println(app.securityArmed ? F("ARMED") : F("DISARMED"));
  }

  // Button 3 — release the door lock, and cancel a sounding alarm. With the
  // alarm silent it toggles the lock.
  if (hal::buttonPressed(hal::Button::Door)) {
    if (app.alarmActive) {
      setActuator(app.alarmActive, false);
      Serial.println(F("[btn] alarm acknowledged"));
    } else {
      setActuator(app.doorLocked, !app.doorLocked);
      app.doorUnlockedAt = app.doorLocked ? 0 : now;
      Serial.print(F("[btn] door "));
      Serial.println(app.doorLocked ? F("locked") : F("unlocked"));
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
void updateOccupancy(unsigned long now) {
  // `lastMotionAt` alone is not enough: millis() also starts at 0, so for the
  // first OCCUPANCY_TIMEOUT_MS `now - 0` is still below the timeout and an
  // empty room would look occupied. motionSeen is the guard for "the PIR has
  // actually reported motion at least once".
  const bool occupied = app.motionSeen &&
                        (now - app.lastMotionAt) < cfg::OCCUPANCY_TIMEOUT_MS;
  setActuator(app.occupied, occupied);
}

// ─────────────────────────────────────────────────────────────────────────────
void updateLighting(unsigned long now) {
  const bool overrideActive =
      app.lightOverride != Override::AUTO && now < app.overrideUntil;

  if (!overrideActive && app.lightOverride != Override::AUTO) {
    // The hold window elapsed, so hand control back to the automatic rule.
    app.lightOverride = Override::AUTO;
    Serial.println(F("[auto] light override expired, back to AUTO"));
  }

  bool lightOn;
  if (overrideActive) {
    lightOn = (app.lightOverride == Override::ON);
  } else {
    lightOn = app.occupied && app.lightLevel < sensors::lightThresholdAdc();
  }
  setActuator(app.lightOn, lightOn);
}

// ─────────────────────────────────────────────────────────────────────────────
void updateFan(unsigned long now) {
  (void)now;
  // An invalid climate reading keeps whatever was decided last; a NaN
  // comparison would otherwise silently switch the fan off.
  if (!app.climateValid) {
    return;
  }
  setActuator(app.fanOn, app.temperature > cfg::TEMP_THRESHOLD_C);
}

// ─────────────────────────────────────────────────────────────────────────────
void updateSecurity(unsigned long now) {
  if (app.securityArmed && app.motionDetected && !app.alarmActive) {
    setActuator(app.alarmActive, true);
    app.lastAlarmAt = now;
  }

  // Self-clearing so a triggered alarm can never latch the system.
  if (app.alarmActive &&
      (now - app.lastAlarmAt) >= cfg::ALARM_MAX_DURATION_MS) {
    setActuator(app.alarmActive, false);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
void updateDoor(unsigned long now) {
  if (!app.doorLocked && app.doorUnlockedAt != 0 &&
      (now - app.doorUnlockedAt) >= cfg::DOOR_UNLOCK_HOLD_MS) {
    setActuator(app.doorLocked, true);
    app.doorUnlockedAt = 0;
    Serial.println(F("[auto] door relocked after hold window"));
  }
}

// ─────────────────────────────────────────────────────────────────────────────
void applyActuators() {
  hal::setRoomLight(app.lightOn);
  hal::setFanIndicator(app.fanOn);
  hal::setSecurityIndicator(app.securityArmed);
  hal::setAlarm(app.alarmActive);
  hal::setDoorLocked(app.doorLocked);
}

// ─────────────────────────────────────────────────────────────────────────────
void reportChanges(unsigned long now) {
  (void)now;
  if (!app.stateChanged) {
    return;
  }
  app.stateChanged = false;

  Serial.print(F("[state] light="));
  Serial.print(app.lightOn ? F("on") : F("off"));
  Serial.print(F(" fan="));
  Serial.print(app.fanOn ? F("on") : F("off"));
  Serial.print(F(" alarm="));
  Serial.print(app.alarmActive ? F("on") : F("off"));
  Serial.print(F(" door="));
  Serial.print(app.doorLocked ? F("locked") : F("unlocked"));
  Serial.print(F(" occupied="));
  Serial.print(app.occupied ? F("yes") : F("no"));
  Serial.print(F(" override="));
  Serial.println(overrideName(app.lightOverride));
}

} // namespace logic