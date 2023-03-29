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

  printSweepValues(PRINT_BASE_VALUES);

  Serial.println("Wait...");
  getVgsOn(pfs->vgmin, pfs->vgmax, (pfs->vginc / 4));

  if (pfs->vgs_on != pfs->vgs_th) {
    pts->k = 2 * pfs->ids_on / (pfs->vgs_th - pfs->vgs_on) /
             (pfs->vgs_th - pfs->vgs_on);
    pts->gm = 2 * pts->k * (pfs->vgs - pfs->vgs_on);
  } else {
    pts->gm = pts->k = 0;
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
  Serial.print(pts->k, 4);
  Serial.print(" gm: ");
  Serial.println(pts->gm, 4);

  pts->VcControl = MAX_VC_VOLTAGE;  // Set to Max for FET

  setVoltage(pts->VccOutPin, pts->VcControl);
  delay(PWM_SETTLE_TIME);

  for (pts->VbControl = pfs->vgmin; pts->VbControl < pfs->vgmax;
       pts->VbControl += (pfs->vginc / 4)) {
    setVoltage(pts->VbOutPin, pts->VbControl);
    delay(PWM_SETTLE_TIME);
    measureVoltages();

    setVoltage(pts->VbOutPin, 0.0);  // Reduce heating
    delay(PWM_SETTLE_TIME);

    if ((pfs->vgs - lastvgs) > 0 && (pfs->id - lastid) > 0 &&
        (pfs->vgs - pfs->vgs_on) > 0) {
      pfs->k =
          2 * pfs->id / (pfs->vgs - pfs->vgs_on) / (pfs->vgs - pfs->vgs_on);
      pts->gm = pfs->k * (pfs->vgs - pfs->vgs_on);
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

  /*
    Serial.println("Wait...");
    getVgsOn(pfs->vgmin, pfs->vgmax, (pfs->vginc / 4));

    if (pfs->vgs_on != pfs->vgs_th) {
      pts->k =
          pfs->ids_on / (pfs->vgs_th - pfs->vgs_on) / (pfs->vgs_th -
    pfs->vgs_on); } else { pts->k = 0;
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
    Serial.println(pts->k, 4);

    pts->VcControl = MAX_VC_VOLTAGE;  // Set to Max for FET

    setVoltage(pts->VccOutPin, pts->VcControl);
    delay(PWM_SETTLE_TIME);

    for (pts->VbControl = pfs->vgmin; pts->VbControl < pfs->vgmax;
         pts->VbControl += (pfs->vginc / 4)) {
      setVoltage(pts->VbOutPin, pts->VbControl);
      delay(PWM_SETTLE_TIME);
      measureVoltages();

      setVoltage(pts->VbOutPin, 0.0);  // Reduce heating
      delay(PWM_SETTLE_TIME);

      if (pts->vgs > pfs->vgs_on) {
        pts->k = pts->id / (pts->vgs - pfs->vgs_on) / (pts->vgs - pfs->vgs_on);
        pts->gm = 2 * pts->k * (pts->vgs - pfs->vgs_on);
        displayVoltages();
      }
    }
    if (flags & EXPORT_DATA) {
      Serial.println("|");
    }
  */
  resetPins();
  clearSweepFlags();
}

void doOutputFETSweep() {
  unsigned char i, j;

  printSweepValues(PRINT_ALL_VALUES);

  pts->VbControl = pfs->vgmin;
  for (i = 0; i < pfs->nvg; i++) {  // Vg
    setVoltage(pts->VbOutPin, pts->VbControl);
    delay(PWM_SETTLE_TIME);

    pts->VcControl = pfs->vdmin;
    for (j = 0; j < pfs->nvd; j++) {  // Vcc
      setVoltage(pts->VccOutPin, pts->VcControl);
      delay(PWM_SETTLE_TIME);

      measureVoltages();

      setVoltage(pts->VccOutPin, 0.0);
      delay(PWM_SETTLE_TIME);

      displayVoltages();

      pts->VcControl += pfs->vdinc;
    }
    Serial.println("=");
    pts->VbControl += pfs->vginc;
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

  pts->VcControl = MAX_VC_VOLTAGE;  // Set to Max for FET
  setVoltage(pts->VccOutPin, pts->VcControl);
  delay(PWM_SETTLE_TIME);

  lastid = 1000;
  thresh = 0.0;
  for (pts->VbControl = min; pts->VbControl < max; pts->VbControl += inc) {
    setVoltage(pts->VbOutPin, pts->VbControl);
    delay(PWM_SETTLE_TIME);
    measureVoltages();

    setVoltage(pts->VbOutPin, 0.0);  // Reduce heating
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
    lastid = pfs->id;
  }
  pts->VbControl = vbcontrol_on;

  resetPins();
}
