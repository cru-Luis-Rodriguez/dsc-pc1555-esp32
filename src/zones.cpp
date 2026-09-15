/*
 *  Direct zone-loop reading — implementation. See include/zones.h.
 *
 *  Reads use analogReadMilliVolts(), which applies the factory ADC calibration
 *  burned into eFuse — raw counts on the ESP32 ADC are notoriously nonlinear.
 *  Each poll takes a 9-sample median per zone to reject transients that the
 *  0.1 µF cap lets through, then requires ZONE_DEBOUNCE_READS consecutive
 *  agreeing classifications before a state change is reported.
 */

#include "zones.h"

// Zone map from the keypad label card (docs/diy-zone-reader.md).
const ZoneConfig kZones[ZONE_COUNT] = {
  { 32, "Front/Rear Door" },
  { 33, "Master Bedroom"  },
  { 34, "2nd Floor"       },
  { 35, "Rear Windows"    },
  { 36, "Front Windows"   },   // SENSOR_VP: needs WiFi.setSleep(false)
};

static ZoneState s_state[ZONE_COUNT];      // debounced
static ZoneState s_pending[ZONE_COUNT];    // candidate awaiting debounce
static uint8_t   s_pendingCount[ZONE_COUNT];
static uint16_t  s_mv[ZONE_COUNT];
static unsigned long s_lastPoll = 0;

const char* zoneStateName(ZoneState s) {
  switch (s) {
    case ZONE_NORMAL: return "closed";
    case ZONE_OPEN:   return "OPEN";
    case ZONE_SHORT:  return "SHORT/tamper";
    default:          return "unknown";
  }
}

static uint16_t readMedianMv(uint8_t pin) {
  uint16_t s[9];
  for (uint8_t i = 0; i < 9; i++) {
    s[i] = analogReadMilliVolts(pin);
    // insertion sort as we go
    for (uint8_t j = i; j > 0 && s[j] < s[j - 1]; j--) {
      uint16_t t = s[j]; s[j] = s[j - 1]; s[j - 1] = t;
    }
  }
  return s[4];
}

static ZoneState classify(uint16_t mv, ZoneState prev) {
  if (mv < ZONE_SHORT_MAX_MV)                          return ZONE_SHORT;
  if (mv >= ZONE_NORMAL_MIN_MV && mv <= ZONE_NORMAL_MAX_MV) return ZONE_NORMAL;
  if (mv >= ZONE_OPEN_MIN_MV)                          return ZONE_OPEN;
  return prev;   // hysteresis band: hold the previous state
}

void zonesBegin() {
  analogReadResolution(12);
  for (uint8_t i = 0; i < ZONE_COUNT; i++) {
    analogSetPinAttenuation(kZones[i].pin, ADC_11db);   // full 0-3.1 V range
    s_mv[i] = readMedianMv(kZones[i].pin);
    s_state[i] = classify(s_mv[i], ZONE_UNKNOWN);
    s_pending[i] = s_state[i];
    s_pendingCount[i] = 0;
  }
  s_lastPoll = millis();
}

bool zonesPoll(void (*onChange)(uint8_t, ZoneState, ZoneState)) {
  if (millis() - s_lastPoll < ZONE_POLL_MS) return false;
  s_lastPoll = millis();

  bool anyChange = false;
  for (uint8_t i = 0; i < ZONE_COUNT; i++) {
    s_mv[i] = readMedianMv(kZones[i].pin);
    ZoneState now = classify(s_mv[i], s_state[i]);

    if (now == s_state[i]) { s_pendingCount[i] = 0; continue; }

    if (now == s_pending[i]) {
      if (++s_pendingCount[i] >= ZONE_DEBOUNCE_READS) {
        ZoneState old = s_state[i];
        s_state[i] = now;
        s_pendingCount[i] = 0;
        anyChange = true;
        if (onChange) onChange(i, old, now);
      }
    } else {
      s_pending[i] = now;
      s_pendingCount[i] = 1;
    }
  }
  return anyChange;
}

ZoneState zoneState(uint8_t idx) { return idx < ZONE_COUNT ? s_state[idx] : ZONE_UNKNOWN; }
uint16_t  zoneMv(uint8_t idx)    { return idx < ZONE_COUNT ? s_mv[idx] : 0; }
