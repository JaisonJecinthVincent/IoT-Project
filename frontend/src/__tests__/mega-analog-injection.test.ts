/**
 * Where the SPICE analog path aims on an ATmega2560.
 *
 * ADC_PIN_TO_GPIO used to hand the Mega the same converter as the Uno: A0 ->
 * pin 14. That is the ATmega328P range, and on a 2560 A0 is pin 54. Two things
 * broke at once, and neither looked like a pin-mapping problem:
 *
 *  - `isPartOwnedPin(boardId, 14)` never matched the sensor that owned pin 54,
 *    so the "a part drives this channel itself, leave it alone" guard missed
 *    and the injector overwrote the part's own value every solve.
 *  - `setAdcWaveform` was handed pin 14, which on a Mega is a plain digital
 *    pin, so a transient solve reached nothing the firmware reads.
 *
 * The symptom was an LDR or potentiometer whose element showed the right lux
 * and whose store properties updated correctly, while analogRead() returned one
 * frozen number no matter what the sensor was set to.
 */
import { describe, it, expect } from 'vitest';
import { gpioForBoardPin } from '../simulation/spice/connectAnalogInputsToMcu';

describe('gpioForBoardPin — AVR analog pads', () => {
  it('maps Uno A0..A5 to the 328P range', () => {
    expect(gpioForBoardPin('arduino-uno', 'A0', 0)).toBe(14);
    expect(gpioForBoardPin('arduino-uno', 'A5', 5)).toBe(19);
  });

  it('maps Mega A0..A15 to 54..69, not to the 328P range', () => {
    expect(gpioForBoardPin('arduino-mega', 'A0', 0)).toBe(54);
    expect(gpioForBoardPin('arduino-mega', 'A1', 1)).toBe(55);
    expect(gpioForBoardPin('arduino-mega', 'A15', 15)).toBe(69);
  });

  it('never hands the Mega a pin that a Mega A-pad does not own', () => {
    // The ownership guard is keyed on this number. If it lands on 14-21 the
    // guard compares against digital pins and can never match.
    for (let channel = 0; channel < 16; channel++) {
      expect(gpioForBoardPin('arduino-mega', `A${channel}`, channel)).toBe(54 + channel);
    }
  });

  it('keeps the ATtiny85 on its own PORTB pads', () => {
    expect(gpioForBoardPin('attiny85', 'PB5', 0)).toBe(14);
  });
});