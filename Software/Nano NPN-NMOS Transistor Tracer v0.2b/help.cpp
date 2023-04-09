#include "help.h"

#include <Arduino.h>
#include <avr/pgmspace.h>

#include "defines.h"
#include "main.h"
#include "nfet.h"
#include "npn.h"
#include "trace.h"

extern struct TRACE_TRANS_STRUT *pts;
extern struct TRACE_FET_STRUT *pfs;

extern unsigned long flags;

#ifdef FLASH_HELP

#ifdef VERBOSE_HELP
const char helpmsg[] PROGMEM = {
    "C\tshow running calibration\r\n"
    "C R\tshow continious ADC values\r\n"
    "C M\tmeasure min and max voltages\r\n"
    "C D\tshow 10 ADC values\r\n"
    "D\tset initial defaults\r\n"
    "E\tshow running ee\r\n"
    "E R\tread ee to current]\r\n"
    "E W\twrite current to ee]\r\n"
    "E I\tinitialize ee]\r\n"
    "M\tshow current voltages\r\n"
    "M R\tshow raw adc voltages\r\n"
    "P [3|10] [255]\tset pin 3 or 10 to pwm 1 to 255\r\n"
    "R\treset\r\n"
    "S V [B|C]\tset Base or Collector control voltage in mV\r\n"
    "S V [B|C] O\tset Base or Collector output voltage in mV\r\n"
    "S U [D|M|U]\tset units to Decimal Milla or Umicro\r\n"
    "S I [B|C] [min] [max]\tset PWM/Input Base or Collector control voltage "
    "min and max sweep "
    "values\r\n"
    "S O [B|C] [min] [max]\tset Output Base or Collector voltage min and max "
    "sweep "
    "values\r\n"
    "S P [B|C] [n]\tset number of Base or Collector sweep values n\r\n"
    "S R [B|C] [n]\tset Base or Collector resistor value n\r\n"
    "T N I\tnpn input trace by Ib\r\n"
    "T N I V\tnpn input trace by Vce\r\n"
    "T N O\tnpn output trace\r\n"
    "T N B\tnpn beta trace by Ib\r\n"
    "T N B C\tnpn beta trace by Ic\r\n"
    "T N R\tnpn re trace\r\n"
    "T N G\tnpn gm trace\r\n"
    "T F I\tfet input trace by Id\r\n"
    "T F I V\tfet input trace by Vgs\r\n"
    "T F O\tfet output trace\r\n"
    "T F K\tfet K constant trace\r\n"
    "T F R\tfet Rds trace\r\n"
    "T F G\tfet gm trace\r\n"
    "T DI\tdiode input trace\r\n"
    "T DO\tdiode output trace\r\n"};

const char guidemsg[] PROGMEM = {
    ""
    ""
    ""
    ""
    ""
    ""
    ""
    ""
    ""};
#else
const char helpmsg[] PROGMEM = {
    "C [R|M]\r\n"
    "D\r\n"
    "E [R|W|I]\r\n"
    "M \r\n"
    "P [3|10] [255]\r\n"
    "R\t\r\n"
    "S V [B|C] [O]\r\n"
    "S U [D|M|U]\r\n"
    "S [I|O] [B|C] [min] [max]\r\n"
    "S P [B|C] [n]\r\n"
    "S R [B|C] [n]\r\n"
    "T N [I|IV|O|B|BC|R|G]\r\n"
    "T F [I|IV|O|K|R|G\r\n"
    "T D [I|O]\r\n"};
#endif

const char bannermsg[] PROGMEM = {"\r\n\r\nTransistor Tracer v0.2b (C)VE3OOI"};

const char cmderr[] PROGMEM = {"CMD Err\r\nEnter H for help\r\n"};
const char numerr[] PROGMEM = {"Num Err"};

void printHelp(void) { pgmMessage(helpmsg); }

void printBanner(void) { pgmMessage(bannermsg); }

void printError(unsigned int errno) {
  switch (errno) {
    case CMDERR:
      pgmMessage(cmderr);
      break;
    case NUMERR:
      pgmMessage(numerr);
      break;
    case ERROR_2:
      pgmMessage(cmderr);
      break;
    case ERROR_3:
      pgmMessage(cmderr);
      break;
    case ERROR_4:
      pgmMessage(cmderr);
      break;
    case ERROR_5:
      pgmMessage(cmderr);
      break;
    case ERROR_6:
      pgmMessage(cmderr);
      break;
  }
}

void pgmMessage(const char *msg) {
  char tmp = 0xd;
  int i = 0;

  while (tmp) {
    tmp = (char)pgm_read_byte(msg + i);
    i++;
    if (tmp) Serial.print(tmp);
  }
  Serial.println("\r\n");
}

#else

void printHelp(char puke) {
  if (puke) Serial.println("** Err **");
  Serial.println("C");              // Display current ADC values
  Serial.println("C [R|M|D]");      // Display ADC values repeatly or
                                    // get/save Max Vb and Vcc or Set default
                                    // calibration values and measure ADC
  Serial.println("D");              // set default values
  Serial.println("E [I|P]");        // init EEPROM or print contents of eeprom
  Serial.println("M [R]");          // Measure voltage or raw ADC values
  Serial.println("P [3|10 [255]");  // Set PWM on pin 3 or pin 10
  Serial.println("R");              // Reset
  Serial.println(
      "S [V] [B|C] [O] [mV]");  // Set voltage on base or Vcc pin.  Set
  output
      // voltage delivered to DUT
      Serial.println("S [I] [uA]");  // Set current om base
  Serial.println("S [U] [D|M|U]");   // Set default unit to decimal, Milla
                                     // or Micro for currents
  Serial.println("S [M] [B|C] [min] [max]");  // Set min max Vb and Vg
                                              // voltages to sweep
  Serial.println("S [R] [B|C] [R]");          // Set Rb or Rc resistor values
  Serial.println("S [P] [B|C] [n]");          // Set number of values to sweep
                                              // between max and min

  Serial.println(
      "T [NI|NIV|NO|NB|NBC|NR|NG]");  // Trace NPN Input, NPN Input by
                                      // voltage, NPN Output, NPN Beta
                                      // w Ib as x, NPN Beta w Ic as x, Ic vs
                                      // Re, Gm vs Ic EFet input, EFet output
  Serial.println("T [FI|FIV|FO|FK|FG|FR]");  // EFET input, EFET input by
                                             // voltage, EFET output, EFET
                                             // K, Gm, Rds
  Serial.println("T [DI|DO]");               // Diode input, Diode output
  using
  // resistor R break;

#endif
