#include "trace.h"

#include <Arduino.h>

#include "defines.h"
#include "main.h"
#include "nfet.h"
#include "npn.h"
#include "trace.h"

extern struct TRACE_TRANS_STRUT *pts;
extern struct TRACE_FET_STRUT *pfs;

extern unsigned long flags;

float ib_u, ic_u, vbe_u, vbc_u, id_u, vce_u, vgs_u, vds_u;

void displayVoltages(void) {
  ib_u = pts->ib;
  ic_u = pts->ic;
  id_u = pfs->id;
  vbe_u = pts->vbe;
  vce_u = pts->vce;
  vgs_u = pfs->vgs;
  vds_u = pfs->vds;
  vbc_u = pts->vbc;
  if (pfs->units == 'M' || pfs->units == 'M') {
    ib_u *= 1000.0;
    ic_u *= 1000.0;
    id_u *= 1000.0;
    vbe_u *= 1000.0;
    vce_u *= 1000.0;
    vbc_u *= 1000.0;
    vgs_u *= 1000.0;
    vds_u *= 1000.0;
  } else if (pfs->units == 'U' || pfs->units == 'U') {
    ib_u *= 1000000.0;
    ic_u *= 1000000.0;
    id_u *= 1000000.0;
    vbe_u *= 1000000.0;
    vce_u *= 1000000.0;
    vbc_u *= 1000000.0;
    vgs_u *= 1000000.0;
    vds_u *= 1000000.0;
  }

  //////////////////////////////////
  ////// SWEEP_BJT_OUTPUT_DATA
  //////////////////////////////////
  if (flags & SWEEP_BJT_OUTPUT_DATA) {
    if (flags & EXPORT_DATA) {
      if (!pts->header) {
        Serial.println("#Ib,Vce,Ic");
        pts->header = 0xff;
      }
      Serial.print("#");
    }

    Serial.print(pts->ib, 12);
    Serial.print(",");
    Serial.print(pts->vce, 12);
    Serial.print(",");
    Serial.println(ic_u, 12);

    //////////////////////////////////
    ////// SWEEP_DIODE_INPUT_DATA
    //////////////////////////////////
  } else if (flags & SWEEP_DIODE_INPUT_DATA) {
    if (flags & EXPORT_DATA) {
      if (!pts->header) {
        Serial.println("#V,I");
        pts->header = 0xff;
      }
      Serial.print("#");
    }
    Serial.print(vbe_u, 20);
    Serial.print(",");
    Serial.println(ib_u, 12);

    //////////////////////////////////
    ////// SWEEP_DIODE_OUTPUT_DATA
    //////////////////////////////////
  } else if (flags & SWEEP_DIODE_OUTPUT_DATA) {
    if (flags & EXPORT_DATA) {
      if (!pts->header) {
        Serial.println("#If,Vf");
        pts->header = 0xff;
      }
      Serial.print("#");
    }
    Serial.print(
        pts->ic,
        20);  // this contains an integer couter to flatten out the curve
    Serial.print(",");
    Serial.println(vbc_u, 12);

    //////////////////////////////////
    ////// SWEEP_BJT_INPUT_DATA
    //////////////////////////////////
  } else if (flags & SWEEP_BJT_INPUT_DATA ||
             flags & SWEEP_BJT_INPUT_DATA_BY_VCE) {
    if (flags & EXPORT_DATA) {
      if (!pts->header) {
        if (flags & SWEEP_BJT_INPUT_DATA) {
          Serial.println("#Vbe,Ic");
        } else {
          Serial.println("#Vbe,Vce");
        }
        pts->header = 0xff;
      }
      Serial.print("#");
    }
    if (flags & SWEEP_BJT_INPUT_DATA) {
      Serial.print(vbe_u, 20);
      Serial.print(",");
      Serial.println(ic_u, 12);
    } else {
      Serial.print(vbe_u, 20);
      Serial.print(",");
      Serial.println(vce_u, 12);
    }

    //////////////////////////////////
    ////// SWEEP_BJT_BETA_DATA
    ////// SWEEP_BJT_BETAC_DATA
    //////////////////////////////////
  } else if (flags & SWEEP_BJT_BETA_DATA || flags & SWEEP_BJT_BETAC_DATA) {
    if (pts->ib == 0) {
      Serial.println("ER");
      return;
    }
    if (flags & EXPORT_DATA) {
      if (!pts->header) {
        if (flags & SWEEP_BJT_BETAC_DATA) {
          Serial.println("#Ic,beta");
        } else {
          Serial.println("#Ib,beta");
        }
        pts->header = 0xff;
      }
      Serial.print("#");
    }

    if (flags & SWEEP_BJT_BETAC_DATA) {
      Serial.print(ic_u, 12);
    } else {
      Serial.print(ib_u, 12);
    }
    Serial.print(",");
    Serial.println(pts->beta, 8);

    //////////////////////////////////
    ////// SWEEP_BJT_RE_DATA
    //////////////////////////////////
  } else if (flags & SWEEP_BJT_RE_DATA) {
    if (flags & EXPORT_DATA) {
      if (!pts->header) {
        Serial.println("#Vbe,Re");
        pts->header = 0xff;
      }
      Serial.print("#");
    }

    Serial.print(pts->vbe, 20);
    Serial.print(",");
    Serial.println(pts->re, 12);

    //////////////////////////////////
    ////// SWEEP_BJT_GM_DATA
    //////////////////////////////////
  } else if (flags & SWEEP_BJT_GM_DATA) {
    if (flags & EXPORT_DATA) {
      if (!pts->header) {
        Serial.println("#Vbe,gm");
        pts->header = 0xff;
      }
      Serial.print("#");
    }
    pts->gm = pts->ic / THERMAL_VOLTAGE;
    Serial.print(pts->vbe, 20);
    Serial.print(",");
    Serial.println(pts->gm, 12);

    //////////////////////////////////
    ////// SWEEP_FET_INPUT_DATA
    //////////////////////////////////
  } else if (flags & SWEEP_FET_INPUT_DATA ||
             flags & SWEEP_FET_VOLTAGE_INPUT_DATA) {
    if (flags & EXPORT_DATA) {
      if (!pts->header) {
        if (flags & SWEEP_FET_INPUT_DATA) {
          Serial.println("#Vgs,Id");
        } else {
          Serial.println("#Vgs,Vds");
        }

        pts->header = 0xff;
      }
      Serial.print("#");
    }

    if (flags & SWEEP_FET_INPUT_DATA) {
      Serial.print(pfs->vgs, 20);
      Serial.print(",");
      Serial.println(id_u, 12);
    } else {
      Serial.print(pfs->vgs, 20);
      Serial.print(",");
      Serial.println(vds_u, 12);
    }

    //////////////////////////////////
    ////// SWEEP_FET_OUTPUT_DATA
    //////////////////////////////////
  } else if (flags & SWEEP_FET_OUTPUT_DATA) {
    if (flags & EXPORT_DATA) {
      if (!pts->header) {
        Serial.println("#Vgs,Vds,Id");
        pts->header = 0xff;
      }
      Serial.print("#");
    }
    Serial.print(pfs->vgs, 12);
    Serial.print(",");
    Serial.print(pfs->vds, 12);
    Serial.print(",");
    Serial.println(id_u, 12);

    //////////////////////////////////
    ////// SWEEP_FET_K_DATA
    //////////////////////////////////
  } else if (flags & SWEEP_FET_K_DATA) {
    if (flags & EXPORT_DATA) {
      if (!pts->header) {
        Serial.println("#Id,k");
        pts->header = 0xff;
      }
      Serial.print("#");
    }
    Serial.print(pfs->id, 12);
    Serial.print(",");
    Serial.println(pts->k, 12);

    //////////////////////////////////
    ////// SWEEP_FET_GM_DATA
    //////////////////////////////////
  } else if (flags & SWEEP_FET_GM_DATA) {
    if (flags & EXPORT_DATA) {
      if (!pts->header) {
        Serial.println("#Id,gm");
        pts->header = 0xff;
      }
      Serial.print("#");
    }
    Serial.print(pfs->id, 12);
    Serial.print(",");
    Serial.println(pts->gm, 12);

    //////////////////////////////////
    ////// SWEEP_FET_R_DATA
    //////////////////////////////////
  } else if (flags & SWEEP_FET_R_DATA) {
    if (flags & EXPORT_DATA) {
      if (!pts->header) {
        Serial.println("#Id,Rds");
        pts->header = 0xff;
      }
      Serial.print("#");
    }

    Serial.print(pfs->id, 12);
    Serial.print(",");
    Serial.println(pfs->rds, 12);

  } else {
    Serial.print(" VccVoltage: ");
    Serial.print(pts->VccVoltage, 4);
    Serial.print(" VcVoltage: ");
    Serial.print(pts->VcVoltage, 4);
    Serial.print(" DeltaV: ");
    Serial.print(pts->vcr, 4);
    Serial.print(" Ic: ");
    Serial.println(pts->ic, 6);

    Serial.print(" Vb1 Voltage: ");
    Serial.print(pts->Vb1Voltage, 4);
    Serial.print(" Vb2 Voltage: ");
    Serial.println(pts->Vb2Voltage, 4);
    Serial.print(" DeltaV: ");
    Serial.print(pts->vbr, 4);
    Serial.print(" Ib: ");
    Serial.println(pts->ib, 6);
    Serial.print("Vbe: ");
    Serial.print(pts->vbe, 4);
    Serial.print(" Beta: ");
    Serial.println(pts->beta, 2);
  }
}

void setUpPWM(void) {
  // Pin 10 is OC1B attached to timer 1
  // Pin 9 is OC2B attached to timer 2
  // Normal timer operation is 900 Hz, need to run at 31KHz
  TCCR1B = ((TCCR1B) & (B11111000)) |
           (B00000001);  // Clear prescalers bits and then set precaler 1 for
                         // timer 1.  Neeed for PWM frequency of 31372.55 Hz
  TCCR2B = ((TCCR2B) & (B11111000)) |
           (B00000001);  // Clear prescalers bits and then set precaler 1 for
                         // timer 2.  Neeed for PWM frequency of 31372.55 Hz
}

void setVoltage(int pin, float voltage) {
  int scaledVoltage = 0;  // voltage x1024 (i.e. shift 10 bits). Max voltage
                          // is 1024x5 = 5120 (i.e 5V)
  int outputValue = 0;    // value output to the PWM (analog out)

  scaledVoltage = int((float)(voltage * (float)VOLTAGE_SCALING));
  outputValue = map(scaledVoltage, 0, MAX_SCALED_VOLTAGE, 0, 255);

  analogWrite(pin, outputValue);
}

int adcAverage(int pin, unsigned char average) {
  unsigned char i;
  unsigned char iterations;
  unsigned long total;

  iterations = 1 << average;

  total = 0;
  for (i = 0; i < iterations; i++) {
    total += analogRead(pin);
  }
  total = total >> average;

  return (int)total;
}

void measureVoltages(void) {
  int temp;

  temp = adcAverage(pts->VccVoltagePin, ADC_BIT_SHIFT_AVERAGE);
  pts->VccVoltage = (float)temp / ADC_CONVERSION;

  temp = adcAverage(pts->VcVoltagePin, ADC_BIT_SHIFT_AVERAGE);
  pts->VcVoltage = (float)temp / ADC_CONVERSION;

  temp = adcAverage(pts->Vb1VoltagePin, ADC_BIT_SHIFT_AVERAGE);
  pts->Vb1Voltage = (float)temp / LOW_ADC_CONVERSION;

  temp = adcAverage(pts->Vb2VoltagePin, ADC_BIT_SHIFT_AVERAGE);
  pts->Vb2Voltage = (float)temp / LOW_ADC_CONVERSION;

  pts->vcr = pts->VccVoltage - pts->VcVoltage;
  pts->vbr = pts->Vb1Voltage - pts->Vb2Voltage;
  pts->vcc = pts->VccVoltage;
  pfs->vgd = pts->vbc = pts->VcVoltage - pts->Vb2Voltage;

  pfs->ig = pts->ib = pts->vbr / (float)pts->rbResistor;
  pfs->id = pts->ic = pts->vcr / (float)pts->rcResistor;
  if (pts->ib > 0) {
    pts->beta = pts->ic / pts->ib;
  } else {
    pts->beta = 0;
  }

  pts->vbe = pfs->vgs = pts->Vb2Voltage;
  pfs->vds = pts->vce = pts->VcVoltage;
  if (pfs->id != 0) {
    pts->re = pfs->rds = pfs->vds / pfs->id;
  } else {
    pts->re = pfs->rds = 0;
  }
}

void getADCValues(void) {
  float temp = 0;
  temp = adcAverage(pts->VccVoltagePin, ADC_BIT_SHIFT_AVERAGE);
  Serial.print("ADC ");
  Serial.print(pts->VccVoltagePin);
  Serial.print(" (Vcc): ");
  Serial.println(temp);

  temp = adcAverage(pts->VcVoltagePin, ADC_BIT_SHIFT_AVERAGE);
  Serial.print("ADC ");
  Serial.print(pts->VcVoltagePin);
  Serial.print(" (Vc): ");
  Serial.println(temp);

  temp = adcAverage(pts->Vb1VoltagePin, ADC_BIT_SHIFT_AVERAGE);
  Serial.print("ADC ");
  Serial.print(pts->Vb1VoltagePin);
  Serial.print(" (Vb1): ");
  Serial.println(temp);

  temp = adcAverage(pts->Vb2VoltagePin, ADC_BIT_SHIFT_AVERAGE);
  Serial.print("ADC ");
  Serial.print(pts->Vb2VoltagePin);
  Serial.print(" (Vcb2): ");
  Serial.println(temp);
}

void getMinMaxVoltages(void) {
  float maxvol = 0;
  float maxcon = 0;
  float minvol = 0;
  float mincon = 0;
  float lastvol = 0;
  float diff = 0;
  unsigned char ctr = 0;
  int pwm = 0;
  int i = 0;
  unsigned char loop = 0;
    unsigned char print = 0;
  pts->VbCalibration = 0;
  pts->VcCalibration = 0;

  Serial.println("Vb");
  for (pwm = 0; pwm < 256; pwm++) {
    analogWrite(pts->VbOutPin, pwm);
    delay(PWM_SETTLE_TIME);
    measureVoltages();
    analogWrite(pts->VbOutPin, 0);  // Reduce heating
    diff = (pts->Vb1Voltage - lastvol);

    pts->VbControl = map(pwm, 0, 255, 0, MAX_SCALED_VOLTAGE);
    pts->VbControl /= (float)VOLTAGE_SCALING;

    if (minvol == 0 && pts->Vb1Voltage > VOLTAGE_ON_THRESHOLD) {
      minvol = pts->Vb1Voltage;
      mincon = pts->VbControl;
    }

    if (maxvol < pts->Vb1Voltage) {
      if (diff > 0 and diff > 0.001) {
        maxvol = pts->Vb1Voltage;
        maxcon = pts->VbControl;
        pts->VbCalibration += (pts->Vb1Voltage / pts->VbControl);
        i++;
        ctr = 0;
      }
    } else if (pts->Vb1Voltage > 0 && (diff < 0 || diff < 0.00001)) {
      if (ctr++ > 10) {
        pwm = 300;
      }
    }

    if (loop++ >= print) {
      Serial.print(loop);
      Serial.println("/255");
      print += 15;
    }
    lastvol = pts->Vb1Voltage;
  }
  if (i > 0) {
    pts->VbCalibration /= (float)i;
    pts->VbControlMin = mincon;
    pts->VbControlMax = maxcon;
    pts->VbControlInc =
        (pts->VbControlMax - pts->VbControlMin) / (float)pts->nvb;
    if (pts->VbControlInc < PWM_RESOLUTION) pts->VbControlInc = PWM_RESOLUTION;
    pts->vbmin = minvol;
    pts->vbmax = maxvol;
    pts->vbinc = (pts->vbmax - pts->vbmin) / (float)pts->nvb;

    Serial.println();
    Serial.print("Min Vb: ");
    Serial.print(minvol);
    Serial.print(" at con: ");
    Serial.println(mincon);

    Serial.print("Max Vb: ");
    Serial.print(maxvol);
    Serial.print(" at con: ");
    Serial.print(maxcon);
    Serial.print(" Cal: ");
    Serial.println(pts->VbCalibration, 3);
    printOutputVoltageValues(PRINT_BASE_VALUES);
    printSweepValues(PRINT_BASE_VALUES);

  } else {
    Serial.println("Vb Err");
  }

  maxvol = 0;
  maxcon = 0;
  minvol = 0;
  mincon = 0;
  lastvol = 0;
  diff = 0;
  ctr = 0;
  i = 0;
  loop = 0;
  print = 0;
  Serial.println("Vc");
  for (pwm = 0; pwm < 256; pwm++) {
    analogWrite(pts->VccOutPin, pwm);
    delay(PWM_SETTLE_TIME);
    measureVoltages();
    analogWrite(pts->VccOutPin, 0);  // Reduce heating
    diff = (pts->VccVoltage - lastvol);

    pts->VcControl = map(pwm, 0, 255, 0, MAX_SCALED_VOLTAGE);
    pts->VcControl /= (float)VOLTAGE_SCALING;

    if (minvol == 0 && pts->VccVoltage > VOLTAGE_ON_THRESHOLD) {
      minvol = pts->VccVoltage;
      mincon = pts->VcControl;
    }

    if (maxvol < pts->VccVoltage) {
      if (diff > 0 and diff > 0.001) {
        maxvol = pts->VccVoltage;
        maxcon = pts->VcControl;
        pts->VcCalibration += (pts->VccVoltage / pts->VcControl);
        i++;
        ctr = 0;
      }

    } else if (pts->Vb1Voltage > 0 && (diff < 0 || diff < 0.00001)) {
      if (ctr++ > 10) {
        pwm = 300;
      }
    }

    if (loop++ >= print) {
      Serial.print(loop);
      Serial.println("/255");
      print += 15;
    }
    lastvol = pts->VccVoltage;
  }
  if (i > 0) {
    pts->VcCalibration /= (float)i;
    pts->VcControlMin = mincon;
    pts->VcControlMax = maxcon;
    pts->VcControlInc =
        (pts->VcControlMax - pts->VcControlMin) / (float)pts->nvc;
    if (pts->VcControlInc < PWM_RESOLUTION) pts->VcControlInc = PWM_RESOLUTION;
    pts->vcmin = minvol;
    pts->vcmax = maxvol;
    pts->vcinc = (pts->vcmax - pts->vcmin) / (float)pts->nvc;
    if (pts->vcinc < PWM_RESOLUTION) pts->vcinc = PWM_RESOLUTION;
    Serial.println();
    Serial.print("Min Vcc: ");
    Serial.print(minvol);
    Serial.print(" at con: ");
    Serial.println(mincon);

    Serial.print("Max Vcc: ");
    Serial.print(maxvol);
    Serial.print(" at con: ");
    Serial.print(maxcon);
    Serial.print(" Cal: ");
    Serial.println(pts->VcCalibration, 3);
    printOutputVoltageValues(PRINT_COLLECTOR_VALUES);
    printSweepValues(PRINT_COLLECTOR_VALUES);
  } else {
    Serial.println("Vc Err");
  }

  resetPins();
  clearSweepFlags();
}

float getVccVoltage(void) {
  float temp;
  temp = adcAverage(pts->PowerOnAnalogPin, ADC_BIT_SHIFT_AVERAGE);
  temp *= VOLTAGE_SENSE_CONVERSION;
  return temp;
}

unsigned char checkVcc(char type) {
  unsigned char bad = 0;

  if (type) {
    // Check if Vcc is powered
    if (getVccVoltage() < MIN_ALLOWED_VCC_VOLTAGE) {
      resetPins();
      clearSweepFlags();
      bad++;
    }
  } else {
    if (!digitalRead(pts->PowerOnDigitalPin)) {
      bad++;
    }
  }
  return bad;
}

void resetPins() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);  // Turn off LED

  pinMode(pts->VccVoltagePin, INPUT);  // ADC input port
  pinMode(pts->VcVoltagePin, INPUT);   // ADC input port
  pinMode(pts->Vb1VoltagePin, INPUT);  // ADC input port
  pinMode(pts->Vb2VoltagePin, INPUT);  // ADC input port
  pinMode(pts->PowerOnAnalogPin, INPUT);

  analogReference(DEFAULT);  // Use 5v reference

  pinMode(pts->VccOutPin, OUTPUT);
  pinMode(pts->VbOutPin, OUTPUT);
  pinMode(pts->PowerOnDigitalPin, INPUT);

  digitalWrite(pts->VccVoltagePin, 0);      // disable pullup
  digitalWrite(pts->VcVoltagePin, 0);       // disable pullup
  digitalWrite(pts->Vb1VoltagePin, 0);      // disable pullup
  digitalWrite(pts->Vb2VoltagePin, 0);      // disable pullup
  digitalWrite(pts->PowerOnDigitalPin, 0);  // disable pullup

  digitalWrite(pts->VccOutPin, 0);  // Zero output
  digitalWrite(pts->VbOutPin, 0);   // Zero output
  setUpPWM();

  setVoltage(pts->VccOutPin, 0);
  setVoltage(pts->VbOutPin, 0);

  delay(PWM_SETTLE_TIME);
}

void resetVoltages(void) {
  pts->VccVoltage = 0;
  pts->VcVoltage = 0;
  pts->Vb1Voltage = 0;
  pts->Vb2Voltage = 0;

  pts->VcControl = 0;
  pts->VbControl = 0;

  pts->vbr = pts->vcr = pts->vc = pts->ve = pts->vcc = 0;
  pts->ic = pts->ib = pts->vce = pts->vbe = pts->vbc = 0;
  pfs->id = pfs->ig = pfs->vgs = pfs->vds = pfs->vgd = pfs->rds;
  pfs->rds_th = pfs->ids_on = pfs->vgs_on = pfs->vgs_th = 0;
  pts->k = pts->gm = pts->beta = 0;
  ib_u = ic_u = vbe_u = vbc_u = id_u = vce_u = vgs_u = vds_u = 0;
}

void setDefaultLimits(void) {
  pts->version = EEPROM_HEADER;
  pts->VccVoltagePin = A4;      // NPN Vcc measure voltage.  Was A1
  pts->VcVoltagePin = A3;       // NPN Vc measure voltage. Was A2
  pts->Vb1VoltagePin = A1;      // NPN Vb1 measure voltage. Was A3
  pts->Vb2VoltagePin = A2;      // NPN Vb2 measure voltage. WAS A4
  pts->VccOutPin = 10;          // PWM NPN Vc controlOutput pin
  pts->VbOutPin = 3;            // PWM NPN Vb control Output pin
  pts->PowerOnDigitalPin = 12;  // Digital sense pin for Vcc is connected
  pts->PowerOnAnalogPin = A0;   // Vcc voltage measurement

  pts->rcResistor = pts->rbResistor = 0;
  pts->VcCalibration = pts->VbCalibration = 0;

  pts->VcControlMin = MIN_VC_VOLTAGE;
  pts->VcControlMax = MAX_VC_VOLTAGE;
  pts->VcControlInc = (pfs->vdmax - pfs->vdmin) / (float)pfs->nvd;
  ;
  pts->VbControlMin = MIN_VB_VOLTAGE;
  pts->VbControlMax = MAX_VB_VOLTAGE;
  pts->VbControlInc = (pfs->vgmax - pfs->vgmin) / (float)pfs->nvg;

  pfs->units = 'D';
  pfs->nvd = NUM_VC_VOLTAGE;
  pfs->nvg = NUM_VB_VOLTAGE;
  pfs->vdmin = 0;
  pfs->vdmax = 0;
  pfs->vdinc = 0;

  pfs->vgmin = 0;
  pfs->vgmax = 0;
  pfs->vginc = 0;

  pfs->VgControl = 0;
  pfs->VdControl = 0;
}

void clearSweepFlags(void) {
  // BJT Flags
  flags &= ~SWEEP_BJT_OUTPUT_DATA;
  flags &= ~SWEEP_FET_OUTPUT_DATA;
  flags &= ~SWEEP_BJT_INPUT_DATA;
  flags &= ~SWEEP_BJT_BETA_DATA;
  flags &= ~SWEEP_BJT_INPUT_DATA_BY_VCE;
  flags &= ~SWEEP_BJT_BETAC_DATA;
  flags &= ~SWEEP_BJT_RE_DATA;
  flags &= ~SWEEP_BJT_GM_DATA;
  // FET Flags
  flags &= ~SWEEP_FET_VOLTAGE_INPUT_DATA;
  flags &= ~SWEEP_FET_K_DATA;
  flags &= ~SWEEP_FET_GM_DATA;
  flags &= ~SWEEP_FET_INPUT_DATA;
  flags &= ~SWEEP_FET_R_DATA;
  // Diode flags
  flags &= ~SWEEP_DIODE_INPUT_DATA;
  flags &= ~SWEEP_DIODE_OUTPUT_DATA;
  // Misc Flags
  flags &= ~PRINT_ADC_VALUES;
  flags &= ~EXPORT_DATA;
}

void printResistorValues(char type) {
  if (type == 'C' || type == 'A') {
    Serial.print("Rc/Rd: ");
    Serial.println(pts->rcResistor);
  }
  if (type == 'B' || type == 'A') {
    Serial.print("Rb/Rg: ");
    Serial.println(pts->rbResistor);
  }
}

unsigned char checkResistors(char type, char announce) {
  unsigned char bad = 0;
  if (type == 'C' || type == 'A') {
    if ((pts->rcResistor == RC_RESISTORLOW ||
         pts->rcResistor == RC_RESISTORHIGH)) {
      if (announce) {
        printResistorValues(PRINT_COLLECTOR_VALUES);
      }
      bad++;
    } else {
      if (announce) {
        Serial.println("Rc/Rd err");
      }
    }
  }
  if (type == 'B' || type == 'A') {
    if (pts->rbResistor == RB_RESISTORLOW ||
        pts->rbResistor == RB_RESISTORHIGH) {
      if (announce) {
        printResistorValues(PRINT_BASE_VALUES);
      }
      bad++;
    } else {
      if (announce) {
        Serial.println("Rb/Rg err");
      }
    }
  }
  return bad;
}

unsigned char checkSweepRange(void) {
  unsigned char bad = 0;
  if (pfs->nvg > 0 && pfs->nvd > 0) {
    bad++;
  }
  if (pfs->vdinc > 0 && pfs->vginc > 0) {
    bad++;
  }
  if (pfs->vdmin >= pfs->vdmax && pfs->vdmax != 0 && pfs->vgmin <= pfs->vgmax &&
      pfs->vgmax != 0) {
    bad++;
  }
  return bad;
}

void printConfig(char type) {
  if (type == 'B' || type == 'A') {
    printOutputVoltageValues(PRINT_BASE_VALUES);
    printSweepValues(PRINT_BASE_VALUES);
    Serial.print("Rb/Rg: ");
    Serial.print(pts->rbResistor);
    Serial.print(" Cal: ");
    Serial.println(pts->VbCalibration);
  }

  if (type == 'C' || type == 'A') {
    printOutputVoltageValues(PRINT_COLLECTOR_VALUES);
    printSweepValues(PRINT_COLLECTOR_VALUES);
    Serial.print("Rc/Rd: ");
    Serial.print(pts->rcResistor);
    Serial.print(" Cal: ");
    Serial.println(pts->VcCalibration);
  }
}

void printSweepValues(char type) {
  if (type == PRINT_BASE_VALUES || type == PRINT_ALL_VALUES) {
    Serial.print("Sweep Vb/Vg: ");
    Serial.print(pts->VbControlMin);
    Serial.print(" - ");
    Serial.print(pts->VbControlMax);
    Serial.print("  points: ");
    Serial.print(pts->nvb);
    Serial.print(" delta: ");
    Serial.println(pts->VbControlInc);
  }

  if (type == PRINT_COLLECTOR_VALUES || type == PRINT_ALL_VALUES) {
    Serial.print("Sweep Vc/Vd: ");
    Serial.print(pts->VcControlMin);
    Serial.print(" - ");
    Serial.print(pts->VcControlMax);
    Serial.print(" points: ");
    Serial.print(pts->nvc);
    Serial.print(" delta: ");
    Serial.println(pts->VcControlInc);
  }

  if (type == PRINT_INPUT_SWEEP_VALUES) {
    Serial.print("Sweep Vb/Vg: ");
    Serial.print(pts->VbControlMin);
    Serial.print(" - ");
    Serial.print(pts->VbControlMax);
    Serial.print("  points: ");
    Serial.print(pts->nvb);
    Serial.print(" delta: ");
    Serial.println(pts->VbControlInc);
    Serial.print("Fixed Vc/Vd: ");
    Serial.println(pts->VcControlMax);
  }
}

void printOutputVoltageValues(char type) {
  if (type == PRINT_BASE_VALUES || type == PRINT_ALL_VALUES) {
    Serial.print("Output Vb/Vg: ");
    Serial.print(pts->vbmin);
    Serial.print(" - ");
    Serial.print(pts->vbmax);
    Serial.print("  points: ");
    Serial.print(pts->nvb);
    Serial.print(" delta: ");
    Serial.println(pts->vbinc);
  }

  if (type == PRINT_COLLECTOR_VALUES || type == PRINT_ALL_VALUES) {
    Serial.print("Output Vc/Vd: ");
    Serial.print(pts->vcmin);
    Serial.print(" - ");
    Serial.print(pts->vcmax);
    Serial.print(" points: ");
    Serial.print(pts->nvc);
    Serial.print(" delta: ");
    Serial.println(pts->vcinc);
  }

  if (type == PRINT_INPUT_SWEEP_VALUES) {
    Serial.print("Output Vb/Vg: ");
    Serial.print(pts->VbControlMin);
    Serial.print(" - ");
    Serial.print(pts->VbControlMax);
    Serial.print("  points: ");
    Serial.print(pts->nvb);
    Serial.print(" delta: ");
    Serial.println(pts->VbControlInc);
    Serial.print("Fixed Vc/Vd: ");
    Serial.println(pts->VcControlMax);
  }
}