/**
 * Smart Building Gateway — the flagship automation example.
 *
 * Arduino Mega 2560, no cloud and no Pro board required. A local edge
 * controller reads the room (DHT22, PIR, LDR), decides what should happen
 * (lighting, fan, security, door lock) and reports every decision on the
 * 16x2 LCD and the Serial Monitor.
 *
 * The firmware is imported with ?raw from ./smartbuilding/ so the code that
 * ships in the gallery is byte-for-byte the code in the repository — there is
 * no second copy to drift out of sync.
 */
import type { ExampleProject } from './examples';

import sketchIno from './smartbuilding/smartbuilding.ino?raw';
import configH from './smartbuilding/config.h?raw';
import stateH from './smartbuilding/state.h?raw';
import stateCpp from './smartbuilding/state.cpp?raw';
import halH from './smartbuilding/hal.h?raw';
import halCpp from './smartbuilding/hal.cpp?raw';
import sensorsH from './smartbuilding/sensors.h?raw';
import sensorsCpp from './smartbuilding/sensors.cpp?raw';
import logicH from './smartbuilding/logic.h?raw';
import logicCpp from './smartbuilding/logic.cpp?raw';
import displayH from './smartbuilding/display.h?raw';
import displayCpp from './smartbuilding/display.cpp?raw';
import appH from './smartbuilding/app.h?raw';
import appCpp from './smartbuilding/app.cpp?raw';

const BOARD = 'arduino-mega';

const RED = '#e02020';
const BLACK = '#1a1a1a';
const BLUE = '#1a7fe0';
const GREEN = '#1aa02a';
const ORANGE = '#e07b1a';
const YELLOW = '#d8c81a';
const PURPLE = '#9a3fd0';

/**
 * Wire endpoints are [componentId, pinName] pairs. The board's own pads use
 * the bare names the Mega element exposes — digits, not 'D22' — and analog
 * pads use 'A0' style names. Getting either wrong silently attaches the wire
 * to the board's top-left corner instead of the pin.
 */
function w(
  id: string,
  from: [string, string],
  to: [string, string],
  color = BLUE,
) {
  return {
    id,
    start: { componentId: from[0], pinName: from[1] },
    end: { componentId: to[0], pinName: to[1] },
    color,
  };
}

export const smartBuildingExamples: ExampleProject[] = [
  {
    id: 'smart-building-gateway',
    title: 'Smart Building Gateway',
    description:
      'Arduino Mega edge controller: DHT22 climate, PIR occupancy, LDR lighting, ' +
      'automatic fan, armable alarm and a servo door lock, reported on a 16x2 LCD.',
    category: 'circuits',
    difficulty: 'advanced',
    boardType: 'arduino-mega',
    libraries: ['DHT sensor library', 'Adafruit Unified Sensor', 'Servo', 'LiquidCrystal'],
    // Order matters: the first entry is the sketch the backend promotes to
    // sketch.ino, and the .cpp files beside it are picked up automatically.
    files: [
      { name: 'smartbuilding.ino', content: sketchIno },
      { name: 'config.h', content: configH },
      { name: 'state.h', content: stateH },
      { name: 'state.cpp', content: stateCpp },
      { name: 'hal.h', content: halH },
      { name: 'hal.cpp', content: halCpp },
      { name: 'sensors.h', content: sensorsH },
      { name: 'sensors.cpp', content: sensorsCpp },
      { name: 'logic.h', content: logicH },
      { name: 'logic.cpp', content: logicCpp },
      { name: 'display.h', content: displayH },
      { name: 'display.cpp', content: displayCpp },
      { name: 'app.h', content: appH },
      { name: 'app.cpp', content: appCpp },
    ],
    // The sketch itself, shown when the files[] array is ignored.
    code: sketchIno,
    components: [
      // ── Sensors ──────────────────────────────────────────────────────────
      { type: 'wokwi-dht22', id: 'dht', x: 530, y: 95, properties: {} },
      { type: 'wokwi-pir-motion-sensor', id: 'pir', x: 530, y: 205, properties: {} },
      {
        type: 'wokwi-photoresistor-sensor',
        id: 'ldr',
        x: 530,
        y: 315,
        // The element defaults (k=5, pullup=10k) squeeze the whole 0..1000 lux
        // range into 1..5% of the ADC span, so a 30% "it is bright" threshold
        // can never be reached and the light never switches off. With the
        // divider solved as R_LDR = dark / (1 + k*lux/1000) above a 100k
        // pull-up, the node sweeps ~23% at 20 lux to ~90% at 900 lux.
        // `dark` must read "1Meg": SPICE parses a bare "1M" as milli-ohms.
        properties: { lux: '500', dark: '1Meg', k: '100', pullup: '100k' },
      },

      // ── Operator controls ────────────────────────────────────────────────
      {
        type: 'wokwi-pushbutton',
        id: 'btn-light',
        x: 520,
        y: 440,
        properties: { label: 'LIGHT' },
      },
      {
        type: 'wokwi-pushbutton',
        id: 'btn-sec',
        x: 640,
        y: 440,
        properties: { label: 'ARM' },
      },
      {
        type: 'wokwi-pushbutton',
        id: 'btn-door',
        x: 760,
        y: 440,
        properties: { label: 'DOOR' },
      },

      // ── Local display ─────────────────────────────────────────────────────
      {
        type: 'wokwi-lcd1602',
        id: 'lcd',
        x: 120,
        y: 330,
        properties: { pins: 'full', background: 'blue', backlight: true },
      },
      {
        type: 'wokwi-potentiometer',
        id: 'contrast',
        x: 400,
        y: 340,
        properties: { value: 55 },
      },

      // ── Actuators ─────────────────────────────────────────────────────────
      {
        type: 'wokwi-servo',
        id: 'door-servo',
        x: 720,
        y: 95,
        properties: { angle: 0, hornColor: '#1a7fe0' },
      },
      { type: 'wokwi-buzzer', id: 'alarm', x: 720, y: 225, properties: {} },

      {
        type: 'wokwi-led',
        id: 'led-light',
        x: 900,
        y: 330,
        properties: { color: 'yellow', label: 'LIGHT' },
      },
      { type: 'wokwi-resistor', id: 'r-light', x: 800, y: 330, properties: { value: '220' } },
      {
        type: 'wokwi-led',
        id: 'led-fan',
        x: 900,
        y: 420,
        properties: { color: 'blue', label: 'FAN' },
      },
      { type: 'wokwi-resistor', id: 'r-fan', x: 800, y: 420, properties: { value: '220' } },
      {
        type: 'wokwi-led',
        id: 'led-sec',
        x: 900,
        y: 510,
        properties: { color: 'red', label: 'ALARM' },
      },
      { type: 'wokwi-resistor', id: 'r-sec', x: 800, y: 510, properties: { value: '220' } },
    ],
    wires: [
      // ── DHT22 climate sensor (D28) ────────────────────────────────────────
      w('w01', ['dht', 'VCC'], [BOARD, '5V'], RED),
      w('w02', ['dht', 'GND'], [BOARD, 'GND.1'], BLACK),
      w('w03', ['dht', 'SDA'], [BOARD, '28'], GREEN),

      // ── PIR motion sensor (D29) ───────────────────────────────────────────
      w('w04', ['pir', 'VCC'], [BOARD, '5V'], RED),
      w('w05', ['pir', 'GND'], [BOARD, 'GND.1'], BLACK),
      w('w06', ['pir', 'OUT'], [BOARD, '29'], YELLOW),

      // ── Photoresistor (A0) ────────────────────────────────────────────────
      w('w07', ['ldr', 'VCC'], [BOARD, '5V'], RED),
      w('w08', ['ldr', 'GND'], [BOARD, 'GND.1'], BLACK),
      w('w09', ['ldr', 'AO'], [BOARD, 'A0'], ORANGE),

      // ── Buttons (D30/D31/D32), internal pull-ups ─────────────────────────
      w('w10', [BOARD, '30'], ['btn-light', '1.l'], GREEN),
      w('w11', ['btn-light', '2.l'], [BOARD, 'GND.2'], BLACK),
      w('w12', [BOARD, '31'], ['btn-sec', '1.l'], GREEN),
      w('w13', ['btn-sec', '2.l'], [BOARD, 'GND.2'], BLACK),
      w('w14', [BOARD, '32'], ['btn-door', '1.l'], GREEN),
      w('w15', ['btn-door', '2.l'], [BOARD, 'GND.2'], BLACK),

      // ── 16x2 LCD in 4-bit mode (D22..D27) ─────────────────────────────────
      w('w16', ['lcd', 'VSS'], [BOARD, 'GND.2'], BLACK),
      w('w17', ['lcd', 'VDD'], [BOARD, '5V'], RED),
      w('w18', ['lcd', 'V0'], ['contrast', 'SIG'], PURPLE),
      w('w19', ['lcd', 'RS'], [BOARD, '22'], BLUE),
      w('w20', ['lcd', 'E'], [BOARD, '23'], BLUE),
      w('w21', ['lcd', 'D4'], [BOARD, '24'], BLUE),
      w('w22', ['lcd', 'D5'], [BOARD, '25'], BLUE),
      w('w23', ['lcd', 'D6'], [BOARD, '26'], BLUE),
      w('w24', ['lcd', 'D7'], [BOARD, '27'], BLUE),

      // ── LCD contrast potentiometer (A1) ───────────────────────────────────
      w('w25', ['contrast', 'VCC'], [BOARD, '5V'], RED),
      w('w26', ['contrast', 'GND'], [BOARD, 'GND.2'], BLACK),
      w('w27', ['contrast', 'SIG'], [BOARD, 'A1'], ORANGE),

      // ── Door lock servo (D9) ──────────────────────────────────────────────
      w('w28', ['door-servo', 'V+'], [BOARD, '5V'], RED),
      w('w29', ['door-servo', 'GND'], [BOARD, 'GND.1'], BLACK),
      w('w30', ['door-servo', 'PWM'], [BOARD, '9'], BLUE),

      // ── Alarm buzzer (D3) ─────────────────────────────────────────────────
      w('w31', [BOARD, '3'], ['alarm', '1'], ORANGE),
      w('w32', ['alarm', '2'], [BOARD, 'GND.1'], BLACK),

      // ── Room light indicator (D4) ─────────────────────────────────────────
      w('w33', [BOARD, '4'], ['r-light', '1'], YELLOW),
      w('w34', ['r-light', '2'], ['led-light', 'A'], YELLOW),
      w('w35', ['led-light', 'C'], [BOARD, 'GND.2'], BLACK),

      // ── Fan indicator (D5) ────────────────────────────────────────────────
      w('w36', [BOARD, '5'], ['r-fan', '1'], BLUE),
      w('w37', ['r-fan', '2'], ['led-fan', 'A'], BLUE),
      w('w38', ['led-fan', 'C'], [BOARD, 'GND.2'], BLACK),

      // ── Security indicator (D6) ───────────────────────────────────────────
      w('w39', [BOARD, '6'], ['r-sec', '1'], RED),
      w('w40', ['r-sec', '2'], ['led-sec', 'A'], RED),
      w('w41', ['led-sec', 'C'], [BOARD, 'GND.2'], BLACK),
    ],
  },
];