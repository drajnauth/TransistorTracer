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

  if (pts->vcmax == 0) {
    pts->VcControl = 2.5;  // Set to half voltage (2.5 ==> Vcc x 0.5)
  } else {
    pts->VcControl = pts->vcmax;  // Set to Max for FET
  }

  printSweepValues(PRINT_BASE_VALUES);

  setVoltage(pts->VccOutPin, pts->VcControl);
  delay(PWM_SETTLE_TIME);

  lastib = lastic = 1000;
  for (pts->VbControl = pts->vbmin; pts->VbControl < pts->vbmax;
       pts->VbControl += (pts->vbinc / 4)) {
    setVoltage(pts->VbOutPin, pts->VbControl);
    delay(PWM_SETTLE_TIME);
    measureVoltages();

    setVoltage(pts->VbOutPin, 0.0);  // Reduce heating
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

  printSweepValues(PRINT_COLLECTOR_VALUES);

  setVoltage(pts->VbOutPin, 0);
  digitalWrite(pts->VbOutPin, 0);
  delay(PWM_SETTLE_TIME);

  avg = i = 0;
  for (pts->VcControl = 0; pts->VcControl < pts->vcmax; pts->VcControl += 0.1) {
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

  if (pts->vbinc > 0.4) pts->vbinc /= 2;

  if (flags & SWEEP_BJT_INPUT_DATA) {
    if (pts->vcmax == 0) {
      pts->VcControl = 2.5;  // Set to half voltage (2.5 ==> Vcc x 0.5)
    } else {
      pts->VcControl = pts->vcmax;  // Set to Max for FET
    }
  } else if (flags & SWEEP_FET_INPUT_DATA) {
    if (pfs->vdmax == 0) {
      pts->VcControl = MAX_VC_VOLTAGE;  // Set to Max for FET
    } else {
      pts->VcControl = pfs->vdmax;  // Set to Max for FET
    }
  }

  printSweepValues(PRINT_BASE_VALUES);

  setVoltage(pts->VccOutPin, pts->VcControl);
  delay(PWM_SETTLE_TIME);

  lastval = 1000;
  for (pts->VbControl = pts->vbmin; pts->VbControl < pts->vbmax;
       pts->VbControl += pts->vbinc) {
    setVoltage(pts->VbOutPin, pts->VbControl);
    delay(PWM_SETTLE_TIME);
    measureVoltages();

    setVoltage(pts->VbOutPin, 0.0);  // Reduce heating
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

  printSweepValues(PRINT_ALL_VALUES);

  pts->VbControl = pts->vbmin;
  for (i = 0; i < pts->nvb; i++) {
    setVoltage(pts->VbOutPin, pts->VbControl);
    delay(PWM_SETTLE_TIME);

    pts->VcControl = pts->vcmin;
    for (j = 0; j < pts->nvc; j++) {  // Vcc Sweep.  Need to ensure same number
                                      // of elements per Vb setting!!!!!
      setVoltage(pts->VccOutPin, pts->VcControl);
      delay(PWM_SETTLE_TIME);

      measureVoltages();
      setVoltage(pts->VccOutPin, 0.0);  // Reduce heating
      delay(PWM_SETTLE_TIME);

      displayVoltages();
      pts->VcControl += pts->vcinc;
    }
    Serial.println("=");
    pts->VbControl += pts->vbinc;
  }

  if (flags & EXPORT_DATA) {
    Serial.println("|");
  }
  resetPins();
  clearSweepFlags();
}
