// ─────────────────────────────────────────────────────────────────────────────
// Smart Building Gateway — configuration
//
// Every tunable in the system lives here. Nothing below is read at runtime,
// so a threshold can be changed by editing one number and re-running.
//
// Pin map (Arduino Mega 2560, fqbn arduino:avr:mega)
//
//   Signal            Pin   Interface
//   ─────────────────────────────────────────────────────────────
//   DHT22 DATA        D28   GPIO, single-wire
//   PIR OUT           D29   GPIO in
//   LDR AO            A0    ADC0
//   Button 1 (light)  D30   GPIO in, pull-up
//   Button 2 (sec)    D31   GPIO in, pull-up
//   Button 3 (door)   D32   GPIO in, pull-up
//   LCD RS            D22   GPIO
//   LCD E             D23   GPIO
//   LCD D4..D7        D24-27 GPIO (4-bit mode)
//   Pot SIG (contrast) A1   ADC1
//   Servo PWM         D9    Timer1 OC1A — Servo library reserves Timer1
//   Buzzer            D3    tone() drives Timer2, so no clash with Servo
//   LED room light    D4    GPIO
//   LED fan           D5    GPIO
//   LED security      D6    GPIO
//
// D0/D1 are left free because they carry the Serial Monitor (USART0).
// GP25-style onboard-LED pins do not apply to the Mega; D13 is spare.
// ─────────────────────────────────────────────────────────────────────────────

#pragma once

#include <Arduino.h>

namespace cfg {

// ── Climate ────────────────────────────────────────────────────────────────
// Above this temperature the fan indicator turns on.
constexpr float TEMP_THRESHOLD_C = 30.0f;
// A DHT22 needs at least 2s between reads; the library returns NaN below that.
constexpr unsigned long CLIMATE_INTERVAL_MS = 2000;

// ── Occupancy ──────────────────────────────────────────────────────────────
// The PIR holds its output high for 3s in the simulator. Occupancy is held
// for this long after the last motion so a single quiet moment does not
// drop the room to empty.
constexpr unsigned long OCCUPANCY_TIMEOUT_MS = 120000UL; // 2 minutes

// ── Lighting ───────────────────────────────────────────────────────────────
// Below this the room counts as dark and the light may be switched on.
//
// The Velxio photoresistor model drives the ADC pin with
// (lux / 1000) * 5V, and the AVR engine converts against a 5V reference, so
// analogRead() is very close to lux * 1024 / 1000 across the full 0..1000
// lux range. The threshold is therefore written in lux and converted once.
constexpr int LIGHT_THRESHOLD_LUX = 250;
constexpr unsigned long LIGHT_SAMPLE_INTERVAL_MS = 100;

// ── Security ───────────────────────────────────────────────────────────────
// An alarm clears itself after this long so a triggered alarm can never
// latch the system permanently. The operator can also cancel it sooner with
// button 3.
constexpr unsigned long ALARM_MAX_DURATION_MS = 10000UL;
// Two pitches alternated while the alarm sounds.
constexpr unsigned int ALARM_TONE_A_HZ = 1800;
constexpr unsigned int ALARM_TONE_B_HZ = 2400;
constexpr unsigned int ALARM_TONE_MS = 250;

// ── Door lock ──────────────────────────────────────────────────────────────
// Servo positions, in degrees. A real strike lock is modelled as the horn
// turning across a 90 degree throw.
constexpr int DOOR_LOCKED_ANGLE = 0;
constexpr int DOOR_UNLOCKED_ANGLE = 90;
// How long the door stays unlocked before it re-locks by itself.
constexpr unsigned long DOOR_UNLOCK_HOLD_MS = 8000UL;
// The servo is nudged to its target over this long so the horn visibly
// travels instead of jumping.
constexpr unsigned long DOOR_MOVE_TIME_MS = 600UL;

// ── Manual override ────────────────────────────────────────────────────────
// A button press suspends the automatic lighting rule for this long, so the
// automation does not immediately undo what the operator just asked for.
constexpr unsigned long OVERRIDE_HOLD_MS = 20000UL;

// ── Display ────────────────────────────────────────────────────────────────
constexpr unsigned long LCD_PAGE_INTERVAL_MS = 3000UL;
// Never redraw the LCD more often than this even if the content changed.
constexpr unsigned long LCD_MIN_REDRAW_MS = 250UL;

// ── Buttons ────────────────────────────────────────────────────────────────
// Contact bounce settles well inside this window.
constexpr unsigned long BUTTON_DEBOUNCE_MS = 40UL;

// Power-on settle window. Presses are ignored until it has elapsed.
//
// avr8js does not emulate INPUT_PULLUP, so the host has to drive the idle
// level itself. Velxio seeds an idle pushbutton pin HIGH, but part
// attachment re-syncs every input pin a little after the CPU is already
// running — measured at ~214ms after reset on this board — which shows up as
// all three buttons going LOW together for long enough to clear a 40ms
// debounce. On real hardware the pull-up holds the line high from the instant
// pinMode() runs, so this window has nothing to reject; it only costs a
// fraction of a second of responsiveness at power-up.
constexpr unsigned long BUTTON_BOOT_GUARD_MS = 600UL;

// ── Base loop period ───────────────────────────────────────────────────────
// Every module is scheduled from this tick so the system never blocks.
constexpr unsigned long TICK_MS = 20UL;

} // namespace cfg