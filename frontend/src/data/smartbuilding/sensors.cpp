#include "sensors.h"

#include "config.h"
#include "hal.h"
#include "state.h"

namespace sensors {
namespace {

unsigned long lastLdrRead = 0;
unsigned long lastClimateAttempt = 0;
bool climateRequested = false;

} // namespace

int lightThresholdAdc() {
  // The photoresistor model drives (lux / 1000) * 5V and the AVR engine
  // converts against a 5V reference, so counts track lux almost linearly.
  // On AVR int is only 16 bits, so the multiply has to be widened.
  return static_cast<int>(
      (static_cast<long>(cfg::LIGHT_THRESHOLD_LUX) * 1024L) / 1000L);
}

void requestImmediateClimate() { climateRequested = true; }

void tick(unsigned long now) {
  // ── Occupancy input is cheap: read it every tick so the 3s PIR hold is
  //    never missed between samples.
  app.motionDetected = hal::readMotion();
  if (app.motionDetected) {
    app.motionSeen = true;
    app.lastMotionAt = now;
  }

  // ── Light level
  if ((now - lastLdrRead) >= cfg::LIGHT_SAMPLE_INTERVAL_MS) {
    lastLdrRead = now;
    app.lightLevel = hal::readLightLevel();
  }

  // ── Climate. The DHT22 refuses to answer faster than every 2s, so a failed
  //    read is expected rather than exceptional; we simply keep the last good
  //    values and leave climateValid false until one succeeds.
  const bool due = climateRequested ||
                   (now - lastClimateAttempt) >= cfg::CLIMATE_INTERVAL_MS;
  if (due) {
    climateRequested = false;
    lastClimateAttempt = now;

    float t = 0.0f;
    float h = 0.0f;
    if (hal::readClimate(t, h)) {
      app.temperature = t;
      app.humidity = h;
      app.climateValid = true;
      app.lastClimateRead = now;
    } else {
      app.climateValid = false;
    }
  }
}

} // namespace sensors