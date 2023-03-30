#ifndef _MAIN_H_
#define _MAIN_H_

#include <arduino.h>

struct TRACE_TRANS_STRUT {  // Structure that stores calibration data
  unsigned char version;
  char units, type, header;
  float rcresistor, rbresistor;
  float vcmin, vcmax, vcinc;  // Vc range
  float vbmin, vbmax, vbinc;  // Vb range
  float VcCalibration, VbCalibration;
  unsigned char nvc;  // number of values to be taken for each pass of Vc
  unsigned char nvb;  // number of values to be taken for each pass of Vb

  int VccVoltagePin;  // NPN Vcc measure voltage
  int VcVoltagePin;   // NPN Vc measure voltage
  int Vb1VoltagePin;  // NPN Vb1 measure voltage
  int Vb2VoltagePin;  // NPN Vb2 measure voltage
  int VccOutPin;      // PWM NPN Vc controlOutput pin
  int VbOutPin;       // PWM NPN Vb control Output pin
  int PowerOnDigitalPin;  // Digital Pin connected to +Vcc power suppy (via
                          // voltage divider)
  int PowerOnAnalogPin;   // ADC Pin connected to +Vcc power suppy (via voltage
                          // divider)

  float VccVoltage;
  float VcVoltage;
  float Vb1Voltage;
  float Vb2Voltage;

  float VcControl;
  float VbControl;

  float vbr, vcr, vc, ve, vcc;  // Same for both
  float beta, re, k, gm;        // Same for both
  float ic, ib, vbe, vce, vbc;  // Only for NPN
  float placeholder1, placeholder2, placeholder3, vbplaceholdere4,
      placeholder5;  // Only for NPN
};

struct TRACE_FET_STRUT {  // Structure that stores calibration data
  unsigned char version;
  char units, type, header;
  float rdresistor, rgresistor;
  float vdmin, vdmax, vdinc;  // Vc range
  float vgmin, vgmax, vginc;  // Vg range
  float VcCalibration, VgCalibration;
  unsigned char nvd;  // number of values to be taken for each pass of Vcc
  unsigned char nvg;  // number of values to be taken for each pass of Vg

  int VccVoltagePin;      // NPN Vcc measure voltage
  int VcVoltagePin;       // NPN Vc measure voltage
  int Vb1VoltagePin;      // NPN Vb1 measure voltage
  int Vb2VoltagePin;      // NPN Vb2 measure voltage
  int VccOutPin;          // PWM NPN Vc controlOutput pin
  int VbOutPin;           // PWM NPN Vb control Output pin
  int PowerOnDigitalPin;  // Digital Pin connected to +Vcc power suppy (via
                          // voltage divider)
  int PowerOnAnalogPin;   // ADC Pin connected to +Vcc power suppy (via voltage
                          // divider)

  float VccVoltage;
  float VcVoltage;
  float Vb1Voltage;
  float Vb2Voltage;

  float VcControl;
  float VbControl;

  float vbr, vcr, vc, ve, vcc;                // Same for both
  float beta, rds, k, gm;                     // Same for both
  float id, ig, vgs, vds, vgd;                // Only for FET
  float ids, rds_th, ids_on, vgs_on, vgs_th;  // Only for NPN
};

void printUnits(void);
void waitForSerial(void);
void Reset(void);
void executeSerial(char* str);
void showHelp(char puke);

#endif  // _MAIN_H_