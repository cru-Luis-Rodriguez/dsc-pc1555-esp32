/*
 *  Raw GPIO probe — is the signal reaching the pin at logic levels?
 *
 *  Answers a question no multimeter can: does GPIO 18 actually cross the
 *  ESP32's logic thresholds, and how often? A DC average says the line moves;
 *  it does not say the edges are seen as edges.
 *
 *  This bypasses dscKeybusInterface entirely. No protocol decoding, no timing
 *  assumptions, no library. Just an interrupt counter and a duty sampler.
 *
 *  Build:  pio run -e probe -t upload
 *  Then:   ~/.platformio/penv/bin/python tools/capture_serial.py --reset \
 *              /dev/cu.usbserial-0001 115200 15 probe.log
 *
 *  Reading the output, one line per second:
 *
 *    clk 0 edges/s, 0.0% high
 *        Pin never crosses threshold. Divider ratio or a broken connection.
 *        If the meter reads ~0.9 V DC here, the line is moving but never
 *        reaching VIH (2.475 V) — the divider is dividing too hard.
 *
 *    clk 0 edges/s, 100.0% high
 *        Pin stuck high. Line not switching, or 33k bypassed.
 *
 *    clk thousands of edges/s, roughly 25-35% high
 *        Levels are fine and the panel is clocking normally. The fault is
 *        then in protocol/timing, not electrical — meaning the panel is
 *        emitting something dscKeybusInterface cannot lock onto, which fits
 *        a panel whose keypads also receive nothing valid.
 *
 *    clk edges/s present but a strange rate or duty
 *        Panel is driving the line with non-standard timing. Record the
 *        numbers; that IS the diagnosis.
 *
 *  Wiring is unchanged from the reader target — this reads the same two pins.
 */

#include <Arduino.h>

#define CLOCK_PIN 18   // panel YEL, via 33k/10k divider
#define DATA_PIN  19   // panel GRN, via 33k/10k divider

volatile uint32_t clockEdges = 0;
volatile uint32_t dataEdges  = 0;

void IRAM_ATTR onClockChange() { clockEdges++; }
void IRAM_ATTR onDataChange()  { dataEdges++; }


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println(F("pin probe: raw edge counter on GPIO 18 (clk) and 19 (data)"));
  Serial.println(F("one line per second; see header comment for interpretation"));

  pinMode(CLOCK_PIN, INPUT);
  pinMode(DATA_PIN,  INPUT);

  attachInterrupt(digitalPinToInterrupt(CLOCK_PIN), onClockChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DATA_PIN),  onDataChange,  CHANGE);
}


void loop() {
  noInterrupts();
  clockEdges = 0;
  dataEdges  = 0;
  interrupts();

  uint32_t samples = 0, clkHigh = 0, datHigh = 0;
  uint32_t start = millis();

  while (millis() - start < 1000) {
    if (digitalRead(CLOCK_PIN)) clkHigh++;
    if (digitalRead(DATA_PIN))  datHigh++;
    samples++;
  }

  noInterrupts();
  uint32_t ce = clockEdges;
  uint32_t de = dataEdges;
  interrupts();

  Serial.printf(
    "clk %8u edges/s %6.1f%% high  |  dat %8u edges/s %6.1f%% high  (%u samples)\n",
    ce, 100.0f * clkHigh / samples,
    de, 100.0f * datHigh / samples,
    samples
  );
}
