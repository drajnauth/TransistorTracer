#include "nfet.h"

#include <Arduino.h>

#include "defines.h"
#include "main.h"
#include "npn.h"
#include "trace.h"

extern struct TRACE_TRANS_STRUT *pts;
extern struct TRACE_FET_STRUT *pfs;

extern unsigned long flags;

void doFETKSweep(void) {
  float lastid = 0;
  float lastvgs = 0;

  printSweepValues(PRINT_INPUT_SWEEP_VALUES);

  getVgsOn(pfs->VgControlMin, pfs->VgControlMax, (pfs->VgControlInc / 4));

  if (pfs->vgs_on != pfs->vgs_th) {
    pfs->k = 2 * pfs->ids_on / (pfs->vgs_th - pfs->vgs_on) /
             (pfs->vgs_th - pfs->vgs_on);
    pfs->gm = 2 * pfs->k * (pfs->vgs - pfs->vgs_on);
  } else {
    pfs->gm = pfs->k = 0;
  }

  Serial.print(" Vgs(th): ");
  Serial.print(pfs->vgs_on, 4);
  Serial.print(" Ids(th): ");
  Serial.print(pfs->ids_on, 4);
  Serial.print(" Vgs(on): ");
  Serial.print(pfs->vgs_th, 4);
  Serial.print(" Rds(on): ");
  Serial.print(pfs->rds_th, 4);
  Serial.print(" k: ");
  Serial.print(pfs->k, 4);
  Serial.print(" gm: ");
  Serial.println(pts->gm, 4);

  pts->VcControl = MAX_VC_VOLTAGE;  // Set to Max for FET

  for (pts->VbControl = pfs->VgControlMin; pts->VbControl < pfs->VgControlMax;
       pts->VbControl += (pfs->VgControlInc / 4)) {
    setVoltage(pts->VccOutPin, pts->VcControl);
    setVoltage(pts->VbOutPin, pts->VbControl);
    delay(PWM_SETTLE_TIME);
    measureVoltages();

    setVoltage(pts->VbOutPin, 0.0);  // Reduce heating
    setVoltage(pts->VccOutPin, 0.0);
    delay(PWM_SETTLE_TIME);

    if ((pfs->vgs - lastvgs) > 0 && (pfs->id - lastid) > 0 &&
        (pfs->vgs - pfs->vgs_on) > 0) {
      pfs->k =
          2 * pfs->id / (pfs->vgs - pfs->vgs_on) / (pfs->vgs - pfs->vgs_on);
      pfs->gm = pfs->k * (pfs->vgs - pfs->vgs_on);
      /*
      // Other equestions
        pts->gm = (pfs->id - lastid) / (pfs->vgs - lastvgs);
        pfs->k = pfs->gm / (pfs->vgs - pfs->vgs_on);
      */
      displayVoltages();
    }
    lastid = pfs->id;
    lastvgs = pfs->vgs;
  }
  if (flags & EXPORT_DATA) {
    Serial.println("|");
  }

  resetPins();
  clearSweepFlags();
}

void doOutputFETSweep() {
  unsigned char i, j;

  printSweepValues(PRINT_ALL_VALUES);

  pts->VbControl = pfs->VgControlMin;
  for (i = 0; i < pfs->nvg; i++) {  // Vg

    pts->VcControl = pfs->VdControlMin;
    for (j = 0; j < pfs->nvd; j++) {  // Vcc
      setVoltage(pts->VbOutPin, pts->VbControl);
      setVoltage(pts->VccOutPin, pts->VcControl);
      delay(PWM_SETTLE_TIME);

      measureVoltages();

      setVoltage(pts->VccOutPin, 0.0);
      setVoltage(pts->VbOutPin, 0.0);
      delay(PWM_SETTLE_TIME);

      displayVoltages();

      pts->VcControl += pfs->VdControlInc;
    }
    Serial.println("=");
    pts->VbControl += pfs->VgControlInc;
  }
  if (flags & EXPORT_DATA) {
    Serial.println("|");
  }
  resetPins();
  clearSweepFlags();
}

void getVgsOn(float min, float max, float inc) {
  float thresh, lastid, vbcontrol_on;
  unsigned char vgsComplete = 0;
  unsigned char ret = 0;
  unsigned int cycles = 0;

  pts->VcControl = MAX_VC_VOLTAGE;  // Set to Max for FET
  cycles = (unsigned int)((max - min) / inc);

  lastid = 1000;
  thresh = 0.0;
  for (pts->VbControl = min; pts->VbControl < max; pts->VbControl += inc) {
    setVoltage(pts->VccOutPin, pts->VcControl);
    setVoltage(pts->VbOutPin, pts->VbControl);
    delay(PWM_SETTLE_TIME);
    measureVoltages();

    setVoltage(pts->VbOutPin, 0.0);  // Reduce heating
    setVoltage(pts->VccOutPin, 0.0);
    delay(PWM_SETTLE_TIME);

    if (lastid == 1000 && pfs->id > 0) lastid = pfs->id;

    if (vgsComplete == 0 && lastid < pfs->id && lastid > 0) {
      thresh = (pfs->id - lastid) / lastid;
      if (thresh >= 0.55) {
        vbcontrol_on = pts->VbControl;
        pfs->vgs_on = pfs->vgs;
        pfs->ids_on = pfs->id;
        vgsComplete = 1;
      }
    } else if (vgsComplete > 0) {
      if (pfs->vds < 1) vgsComplete++;
      if (vgsComplete > 2) {
        pfs->rds_th = pfs->rds;
        pfs->vgs_th = pfs->vgs;
        pts->VbControl = max + 1;
      }
    }
    if (ret++ > 16) {
      Serial.print(ret);
      Serial.print("/");
      Serial.println(cycles);
    }
    lastid = pfs->id;
  }
  pts->VbControl = vbcontrol_on;

  resetPins();
}
