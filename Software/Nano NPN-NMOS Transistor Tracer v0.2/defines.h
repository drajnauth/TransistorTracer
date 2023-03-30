
#ifndef _DEFINES_H_
#define _DEFINES_H_

#include <arduino.h>

#define DEBUG_ENABLE

#define BANNER "\r\n\r\nTransistor Tracer v0.1b (C)VE3OOI"

#define EEPROM_HEADER 0xA1

/*
ADC Calibration

1) Apply 5 volts to each POT
2) Set voltage read at wiper to about 1.75
3) Connect Arduino and read ADC values
4) Adjust pot to get about 364
5) Calculate divider, adc_conv, v_conv
6) Change ADC_CONVERSION to be v_conv
7) if Pots are changes must redo this

Example:
ADC4, ADC3, ADC2, ADC1 = comma separated
Vapplied: 4.9V,4.9,4.76,4.86
Vadc: 1.755,1.753,1.701,1.74
ADC:364,364,353,362

Divider(avg) = Vadc/Vapplied =  0.3575
ADC_Conv(AVG) = ADC/Vadc = 207.7
V_conv = Divider*ADC_Conv = 74.22
Vmeasured = ADC / V_conv
*/

#define ADC_CONVERSION ((float)74.22)

#define LOW_ADC_CONVERSION ((float)74.22)

// #define PWM_SCALING  ((float)11.3 / (float)4.6)  // was ((float)11.3 /
// (float)4.6)

// Need to scale the voltage to an PWM signal which is 0-255 (8 bits)
// Make some assumptions.  Make max voltage 598 and scale values relative to
// this.
#define VOLTAGE_SCALING 128
#define MAX_SCALED_VOLTAGE (589)  // 4.6*128

/*
Resistor divider is 991 and 497. Divider is 497/(497+991)=0.3340
ADC Voltage conversion is 4.88V/1024 so 1 ADC count is 4.766mV
With resistive divider each count is actually 4.766mV/0.3340 = 14.26mV =
0.01426V
*/
#define VOLTAGE_SENSE_CONVERSION \
  0.01426  // V per count.  Multiply by ADC counts to get voltage in V
#define MAX_ALLOWED_VCC_VOLTAGE \
  14  // Any voltage above this could damage arduino
#define MIN_ALLOWED_VCC_VOLTAGE \
  8  // Arbituary set.  Vcc of 9 volts is barely usable

#define MAX_VC_VOLTAGE 4.6
#define MIN_VC_VOLTAGE 0.010
#define NUM_VC_VOLTAGE 15
#define MIN_NUM_VC_VOLTAGE 5
#define MAX_NUM_VC_VOLTAGE 20
#define ERROR_VC_VOLTAGE 0.009

#define MAX_VB_VOLTAGE 4.6
#define MIN_VB_VOLTAGE 0
#define NUM_VB_VOLTAGE 10
#define MIN_NUM_VB_VOLTAGE 5
#define MAX_NUM_VB_VOLTAGE 20

#define CALIBRATION_VC_VOLTAGE 4.6
#define CALIBRATION_VB_VOLTAGE 2.3

#define THERMAL_VOLTAGE 0.026

#define RC_RESISTORHIGH 32
#define RC_RESISTORLOW 10
#define RB_RESISTORHIGH 1000
#define RB_RESISTORLOW 462

#define PWM_SETTLE_TIME 40  // Was 30ms
#define ADC_BIT_SHIFT_AVERAGE 7

#define PRINT_BASE_VALUES 'B'
#define PRINT_COLLECTOR_VALUES 'C'
#define PRINT_ALL_VALUES 'A'

#define QUIET 0
#define PUKE 1
#define CHECK_ANALOG_VCC 1
#define CHECK_DIGITAL_VCC 0

#define SWEEP_BJT_INPUT_DATA 0x00000001
#define SWEEP_BJT_OUTPUT_DATA 0x00000002
#define SWEEP_BJT_INPUT_DATA_BY_VCE 0x00000004
#define SWEEP_BJT_BETA_DATA 0x00000008
#define SWEEP_BJT_BETAC_DATA 0x00000010
#define SWEEP_BJT_RE_DATA 0x00000020
#define SWEEP_BJT_GM_DATA 0x00000040
///////////////
#define SWEEP_FET_INPUT_DATA 0x00000080
#define SWEEP_FET_OUTPUT_DATA 0x00000100
#define SWEEP_FET_VOLTAGE_INPUT_DATA 0x00000200
#define SWEEP_FET_K_DATA 0x00000400
#define SWEEP_FET_GM_DATA 0x00000800
#define SWEEP_FET_R_DATA 0x00001000
//////////////
#define SWEEP_DIODE_INPUT_DATA 0x00002000
#define SWEEP_DIODE_OUTPUT_DATA 0x00004000
//////////////
#define PRINT_ADC_VALUES 0x00100000
#define EXPORT_DATA 0x00200000

#endif  // _DEFINES_H_