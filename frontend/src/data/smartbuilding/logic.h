// ─────────────────────────────────────────────────────────────────────────────
// Smart Building Gateway — control rules
//
// Every automatic decision in the system is in this file. It reads AppState
// and writes the desired actuator values back into AppState; it never drives
// hardware itself, which is what lets all the rules see one consistent
// snapshot when several inputs change in the same tick.
//
// Precedence notes
//   * Lighting and the fan are decided independently: a dark room can be lit
//     while still cool, and a hot room can be ventilated while still light.
//   * The fan follows temperature alone. Occupancy never switches it off,
//     because leaving an overheated room unventilated is worse than the small
//     energy cost of an empty one. Test case "empty room" therefore expects
//     the fan off only at a normal temperature.
//   * A failed sensor read never clears an actuator. The last good decision
//     stands and the display reports the reading as stale.
// ─────────────────────────────────────────────────────────────────────────────

#pragma once

#include <Arduino.h>

namespace logic {

/// Read the buttons and translate presses into operator intent.
void handleButtons(unsigned long now);

void updateOccupancy(unsigned long now);
void updateLighting(unsigned long now);
void updateFan(unsigned long now);
void updateSecurity(unsigned long now);
void updateDoor(unsigned long now);

/// Push the decided actuator values out to the hardware.
void applyActuators();

/// Report every state change once, over Serial.
void reportChanges(unsigned long now);

} // namespace logic