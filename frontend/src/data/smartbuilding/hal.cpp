#include "hal.h"

#include "config.h"

#include <DHT.h>
#include <LiquidCrystal.h>
#include <Servo.h>

namespace hal {
namespace {

// ── Pin assignments (see config.h for the table) ────────────────────────────
constexpr uint8_t PIN_LIGHT_LED = 4;
constexpr uint8_t PIN_FAN_LED = 5;
constexpr uint8_t PIN_SECURITY_LED = 6;
constexpr uint8_t PIN_BUZZER = 3;
constexpr uint8_t PIN_SERVO = 9; // Timer1 OC1A — Servo.h claims Timer1
constexpr uint8_t PIN_LDR = A0;
constexpr uint8_t PIN_PIR = 29;
constexpr uint8_t PIN_DHT = 28;
constexpr uint8_t PIN_BTN_LIGHT = 30;
constexpr uint8_t PIN_BTN_SECURITY = 31;
constexpr uint8_t PIN_BTN_DOOR = 32;
constexpr uint8_t PIN_LCD_RS = 22;
constexpr uint8_t PIN_LCD_EN = 23;
constexpr uint8_t PIN_LCD_D4 = 24;
constexpr uint8_t PIN_LCD_D5 = 25;
constexpr uint8_t PIN_LCD_D6 = 26;
constexpr uint8_t PIN_LCD_D7 = 27;

Servo doorServo;
LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6,
                  PIN_LCD_D7);
DHT climate(PIN_DHT, DHT22);

// ── Button debounce ────────────────────────────────────────────────────────
struct DebouncedButton {
  uint8_t pin;
  bool stable;      ///< last accepted level, HIGH = idle
  bool candidate;   ///< level seen during the current debounce window
  bool edge;        ///< TRUE for exactly one tick after a new press
  bool haveSample;  ///< at least one reading has been taken
  bool idleReady;   ///< line has held HIGH for a full debounce window
  unsigned long changedAt;  ///< when candidate last flipped
  unsigned long highSince;  ///< when the line last settled at HIGH

  void begin(uint8_t p) {
    pin = p;
    stable = true; // idle is HIGH because of the pull-up
    candidate = true;
    edge = false;
    haveSample = false;
    idleReady = false;
    changedAt = 0;
    highSince = 0;
    pinMode(pin, INPUT_PULLUP);
  }

  void update(unsigned long now) {
    const bool level = digitalRead(pin) == HIGH;

    // The very first reading only establishes a baseline. Nothing is trusted
    // yet and no transition is derived from it: until the pin has been
    // sampled once there is no "previous" level to have changed from.
    if (!haveSample) {
      haveSample = true;
      stable = level;
      candidate = level;
      edge = false;
      highSince = now;
      return;
    }

    if (level != candidate) {
      candidate = level;
      changedAt = now;
      if (level) {
        highSince = now;
      }
      return;
    }

    if (candidate != stable) {
      // The level has to hold for the whole window before it is believed.
      if ((now - changedAt) < cfg::BUTTON_DEBOUNCE_MS) {
        return;
      }
      stable = candidate;
      if (stable) {
        highSince = now;
      } else {
        // Settled LOW: a press — but only once the line has demonstrably been
        // at rest for a full debounce window, and never during the power-on
        // settle window. avr8js does not emulate INPUT_PULLUP, so the host
        // drives the idle level itself, and its re-sync of every input pin
        // shortly after reset looks exactly like several buttons pressed at
        // once. Both guards are no-ops on real hardware, where the pull-up
        // holds the line high from the first instruction of setup().
        if (idleReady && now >= cfg::BUTTON_BOOT_GUARD_MS) {
          edge = true;
        }
        idleReady = false;
      }
      return;
    }

    // candidate == stable == HIGH, still inside the settling window.
    if (stable && !idleReady &&
        (now - highSince) >= cfg::BUTTON_DEBOUNCE_MS) {
      idleReady = true;
    }
  }

  void consumeEdge() { edge = false; }
};

DebouncedButton btnLight;
DebouncedButton btnSecurity;
DebouncedButton btnDoor;

// ── Door servo easing ──────────────────────────────────────────────────────
int doorAngle = 0;          ///< where the horn actually is, in degrees
int doorTargetAngle = 0;    ///< where it is heading
unsigned long doorStepAt = 0;
bool alarmOn = false;
unsigned long alarmStepAt = 0;
bool alarmPitchHigh = false;

/// Convert the configured lux threshold into the ADC counts the AVR engine
/// produces for it. See the calibration note in config.h.
int luxToAdc(int lux) {
  // int is 16 bits on AVR, so the multiply has to be widened or it overflows.
  return static_cast<int>((static_cast<long>(lux) * 1024L) / 1000L);
}

void writeDoorServo() {
  doorServo.write(constrain(doorAngle, 0, 180));
  doorStepAt = millis();
}

/// Advance the servo one small step toward its target.
void serviceDoorServo(unsigned long now) {
  if (doorAngle == doorTargetAngle) {
    return;
  }
  if ((now - doorStepAt) < (cfg::DOOR_MOVE_TIME_MS / 12UL)) {
    return;
  }
  doorAngle += (doorTargetAngle > doorAngle) ? 3 : -3;
  doorAngle = constrain(doorAngle, 0, 180);
  writeDoorServo();
}

/// Alternate the alarm between two pitches.
void serviceAlarm(unsigned long now) {
  if (!alarmOn) {
    return;
  }
  if ((now - alarmStepAt) < cfg::ALARM_TONE_MS) {
    return;
  }
  alarmStepAt = now;
  alarmPitchHigh = !alarmPitchHigh;
  tone(PIN_BUZZER, alarmPitchHigh ? cfg::ALARM_TONE_A_HZ : cfg::ALARM_TONE_B_HZ);
}

} // namespace

void init() {
  pinMode(PIN_LIGHT_LED, OUTPUT);
  pinMode(PIN_FAN_LED, OUTPUT);
  pinMode(PIN_SECURITY_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_LIGHT_LED, LOW);
  digitalWrite(PIN_FAN_LED, LOW);
  digitalWrite(PIN_SECURITY_LED, LOW);

  btnLight.begin(PIN_BTN_LIGHT);
  btnSecurity.begin(PIN_BTN_SECURITY);
  btnDoor.begin(PIN_BTN_DOOR);

  doorServo.attach(PIN_SERVO);
  doorAngle = cfg::DOOR_LOCKED_ANGLE;
  doorTargetAngle = doorAngle;
  writeDoorServo();

  lcd.begin(16, 2);
  climate.begin();

  Serial.begin(115200);
  Serial.println(F("[hal] smart building gateway ready"));
  Serial.print(F("[hal] light threshold "));
  Serial.print(luxToAdc(cfg::LIGHT_THRESHOLD_LUX));
  Serial.print(F(" adc counts ("));
  Serial.print(cfg::LIGHT_THRESHOLD_LUX);
  Serial.println(F(" lux)"));
}

int readLightLevel() { return analogRead(PIN_LDR); }

bool readMotion() { return digitalRead(PIN_PIR) == HIGH; }

bool readClimate(float &t, float &h) {
  t = climate.readTemperature();
  h = climate.readHumidity();
  if (isnan(t) || isnan(h)) {
    return false;
  }
  return true;
}

bool buttonPressed(Button b) {
  bool *edge = nullptr;
  switch (b) {
  case Button::Light:
    edge = &btnLight.edge;
    break;
  case Button::Security:
    edge = &btnSecurity.edge;
    break;
  case Button::Door:
    edge = &btnDoor.edge;
    break;
  }
  if (edge == nullptr || !*edge) {
    return false;
  }
  *edge = false;
  return true;
}

void setRoomLight(bool on) { digitalWrite(PIN_LIGHT_LED, on ? HIGH : LOW); }

void setFanIndicator(bool on) { digitalWrite(PIN_FAN_LED, on ? HIGH : LOW); }

void setSecurityIndicator(bool on) {
  digitalWrite(PIN_SECURITY_LED, on ? HIGH : LOW);
}

void setAlarm(bool on) {
  if (on == alarmOn) {
    return;
  }
  alarmOn = on;
  if (on) {
    alarmPitchHigh = false;
    tone(PIN_BUZZER, cfg::ALARM_TONE_B_HZ);
    alarmStepAt = millis();
  } else {
    noTone(PIN_BUZZER);
  }
}

void setDoorLocked(bool locked) {
  doorTargetAngle =
      locked ? cfg::DOOR_LOCKED_ANGLE : cfg::DOOR_UNLOCKED_ANGLE;
}

/// Called every base tick from the orchestrator.
void service(unsigned long now) {
  btnLight.update(now);
  btnSecurity.update(now);
  btnDoor.update(now);
  serviceDoorServo(now);
  serviceAlarm(now);
}

// ── Display ────────────────────────────────────────────────────────────────

namespace {
constexpr uint8_t LCD_COLS = 16;
constexpr uint8_t LCD_ROWS = 2;
/// Mirrors what is on the panel so display.cpp can skip redundant writes.
char lcdShadow[LCD_ROWS][LCD_COLS + 1] = {{0}, {0}};
} // namespace

void lcdInit() { lcd.begin(LCD_COLS, LCD_ROWS); }

void lcdClear() {
  lcd.clear();
  for (uint8_t r = 0; r < LCD_ROWS; r++) {
    lcdShadow[r][0] = '\0';
  }
}

void lcdWriteRow(uint8_t row, const char *text) {
  if (row >= LCD_ROWS || text == nullptr) {
    return;
  }
  lcd.setCursor(0, row);
  for (uint8_t col = 0; col < LCD_COLS; col++) {
    const char c = (text[col] == '\0') ? ' ' : text[col];
    lcd.write((uint8_t)c);
    lcdShadow[row][col] = c;
  }
  lcdShadow[row][LCD_COLS] = '\0';
}

void lcdCreateChar(uint8_t index, const uint8_t bitmap[8]) {
  lcd.createChar(index, const_cast<uint8_t *>(bitmap));
}

} // namespace hal