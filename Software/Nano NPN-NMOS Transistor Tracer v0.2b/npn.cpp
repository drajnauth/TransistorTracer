#include "npn.h"

#include <Arduino.h>

#include "defines.h"
#include "main.h"
#include "nfet.h"
#include "trace.h"

extern struct TRACE_TRANS_STRUT *pts;
extern struct TRACE_FET_STRUT *pfs;

extern unsigned long flags;

void doNPNBetaSweep(void) {
  float lastib, lastic;

  printControlValues(INPUT_SWEEP_VALUES);
  printOutputVoltageValues(INPUT_SWEEP_VALUES);
  pts->VcControl = pts->VcControlMax;

  lastib = lastic = 1000;
  for (pts->VbControl = pts->VbControlMin; pts->VbControl < pts->VbControlMax;
       pts->VbControl += (pts->VbControlInc / 4)) {
    setVoltage(pts->VccOutPin, pts->VcControl);
    setVoltage(pts->VbOutPin, pts->VbControl);
    delay(PWM_SETTLE_TIME);
    measureVoltages();

    setVoltage(pts->VbOutPin, 0.0);  // Reduce heating
    setVoltage(pts->VccOutPin, 0.0);
    delay(PWM_SETTLE_TIME);

    if (lastib != pts->ib && lastic != pts->ic && pts->beta > 0 &&
        pts->ib > 1e-6 && pts->ic > 10e-6 && pts->vbe > 0.3) {
      displayVoltages();
    }
    lastib = pts->ib;
    lastic = pts->ic;
  }
  if (flags & EXPORT_DATA) {
    Serial.println("|");
  }

  resetPins();
  clearSweepFlags();
}

void doDiodeOutputSweep(void) {
  int i;
  float avg;

  printControlValues(COLLECTOR_VALUES);
    printOutputVoltageValues(COLLECTOR_VALUES);

  setVoltage(pts->VbOutPin, 0);
  digitalWrite(pts->VbOutPin, 0);
  delay(PWM_SETTLE_TIME);

  avg = i = 0;
  for (pts->VcControl = 0; pts->VcControl < pts->VcControlMax;
       pts->VcControl += 0.1) {
    setVoltage(pts->VccOutPin, pts->VcControl);
    delay(PWM_SETTLE_TIME);
    measureVoltages();

    setVoltage(pts->VccOutPin, 0.0);  // Reduce heating
    delay(PWM_SETTLE_TIME);

    pts->vbe = (float)i;
    avg = avg / 2 + pts->vbc / 2;
    displayVoltages();
    i++;
  }

  if (flags & EXPORT_DATA) {
    Serial.println("|");
  }

  Serial.print("Vf: ");
  Serial.println(avg, 4);

  resetPins();
  clearSweepFlags();
}

void doNPNInputSweep() {
  float lastval;

  printControlValues(INPUT_SWEEP_VALUES);    printOutputVoltageValues(INPUT_SWEEP_VALUES);
  pts->VcControl = pts->VcControlMax;

  lastval = 1000;
  for (pts->VbControl = pts->VbControlMin; pts->VbControl < pts->VbControlMax;
       pts->VbControl += pts->VbControlInc) {
    setVoltage(pts->VccOutPin, pts->VcControl);
    setVoltage(pts->VbOutPin, pts->VbControl);
    delay(PWM_SETTLE_TIME);
    measureVoltages();

    setVoltage(pts->VbOutPin, 0.0);  // Reduce heating
    setVoltage(pts->VccOutPin, 0.0);
    delay(PWM_SETTLE_TIME);

    if (lastval != pts->ib) {
      displayVoltages();
    }
    lastval = pts->ib;
  }
  if (flags & EXPORT_DATA) {
    Serial.println("|");
  }

  resetPins();
  clearSweepFlags();
}

void doOutputNPNSweep() {
  unsigned char i, j;

  printControlValues(ALL_VALUES);
  printOutputVoltageValues(ALL_VALUES);

  pts->VbControl = pts->VbControlMin;
  for (i = 0; i < pts->nvb; i++) {
    pts->VcControl = pts->VcControlMin;
    for (j = 0; j < pts->nvc; j++) {  // Vcc Sweep.  Need to ensure same number
                                      // of elements per Vb setting!!!!!
      setVoltage(pts->VbOutPin, pts->VbControl);
      setVoltage(pts->VccOutPin, pts->VcControl);
      delay(PWM_SETTLE_TIME);

      measureVoltages();
      setVoltage(pts->VccOutPin, 0.0);  // Reduce heating
      setVoltage(pts->VbOutPin, 0.0);
      delay(PWM_SETTLE_TIME);

      displayVoltages();
      pts->VcControl += pts->VcControlInc;
    }
    Serial.println("=");
    pts->VbControl += pts->VbControlInc;
  }

  if (flags & EXPORT_DATA) {
    Serial.println("|");
  }
  resetPins();
  clearSweepFlags();
}
