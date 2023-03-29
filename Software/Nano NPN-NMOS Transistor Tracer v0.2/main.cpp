#include "main.h"

#include <Arduino.h>
#include <EEPROM.h>

#include "UART.h"  // VE3OOI Serial Interface Routines (TTY Commands)
#include "defines.h"
#include "nfet.h"
#include "npn.h"
#include "trace.h"

// These variables are defined in UART.cpp and used for Serial interface
// rbuff is used to store all keystrokes which is parsed by Execute()
// commands[] and numbers[] store all characters or numbers entered provided
// they are separated by spaces. ctr is counter used to process entries
// command_entries contains the total number of charaters/numbers entered
extern char rbuff[RBUFF];
extern char commands[MAX_COMMAND_ENTRIES];
extern unsigned char command_entries;
extern unsigned long numbers[MAX_COMMAND_ENTRIES];
extern unsigned char ctr;

extern char prompt[6];
extern char ovflmsg[9];
extern char errmsg[4];

unsigned long flags = 0;
unsigned int errorcode = 0;

int temp = 0;
int i = 0;
unsigned int addr;  // EEPROM starting address for calibration strut

struct TRACE_TRANS_STRUT *pts;
struct TRACE_FET_STRUT *pfs;

/*
#include <EEPROM.h>
  addr = 0;
  EEPROM.put(addr, st);
    memset((char *)&st, 0, sizeof(st));
    EEPROM.get(addr, st);
*/

void setup() {
  Serial.begin(9600);

  pfs = (TRACE_FET_STRUT *)malloc(sizeof(TRACE_FET_STRUT));
  pts = (TRACE_TRANS_STRUT *)pfs;
  memset((char *)pfs, 0, sizeof(TRACE_FET_STRUT));

  Reset();

  addr = 0;
  EEPROM.get(addr, *pfs);

  if (pfs->version != EEPROM_HEADER) {
    if (checkVcc(CHECK_ANALOG_VCC)) {
      Serial.println("No Vcc. EE INIT Err");
    } else {
      Serial.println("EE INIT");
      setDefaultLimits();
      pts->rcresistor = RC_RESISTORHIGH;
      pts->rbresistor = RB_RESISTORLOW;
      getMaxVoltages();
      EEPROM.put(addr, *pfs);
    }
  } else {
    Serial.println("EE OK");
    printConfig(PRINT_ALL_VALUES);
  }
  Serial.write(prompt);
}

void loop() {
  if (checkVcc(CHECK_DIGITAL_VCC)) {
    Serial.println("Vcc Off");
    for (i = 0; i < 5; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(250);
      digitalWrite(LED_BUILTIN, LOW);
      delay(250);
    }
  } else {
    // Look for characters entered from the keyboard and process them
    // This function is part of the UART package.
    processSerial();
  }

  if (flags & PRINT_ADC_VALUES) {
    temp = adcAverage(pts->VccVoltagePin, ADC_BIT_SHIFT_AVERAGE);
    Serial.print(temp);
    Serial.print("\t");

    temp = adcAverage(pts->VcVoltagePin, ADC_BIT_SHIFT_AVERAGE);
    Serial.print(temp);
    Serial.print("\t");

    temp = adcAverage(pts->Vb1VoltagePin, ADC_BIT_SHIFT_AVERAGE);
    Serial.print(temp);
    Serial.print("\t");

    temp = adcAverage(pts->Vb2VoltagePin, ADC_BIT_SHIFT_AVERAGE);
    Serial.println(temp);

  } else if (flags & SWEEP_BJT_OUTPUT_DATA) {
    Serial.println("BJT Output Sweep");
    printUnits();
    doOutputNPNSweep();
    Serial.write(prompt);

  } else if (flags & SWEEP_BJT_INPUT_DATA ||
             flags & SWEEP_BJT_INPUT_DATA_BY_VCE) {
    Serial.println("BJT Input Sweep");
    printUnits();
    doNPNInputSweep();
    Serial.write(prompt);

  } else if (flags & SWEEP_BJT_BETA_DATA || flags & SWEEP_BJT_BETAC_DATA ||
             flags & SWEEP_BJT_RE_DATA || flags & SWEEP_BJT_GM_DATA) {
    Serial.println("NPN Beta/Re/Gm Sweep");
    printUnits();
    doNPNBetaSweep();
    Serial.write(prompt);

  } else if (flags & SWEEP_DIODE_INPUT_DATA) {
    Serial.println("Diode Input Sweep");
    printUnits();
    doNPNInputSweep();
    Serial.write(prompt);

  } else if (flags & SWEEP_DIODE_OUTPUT_DATA) {
    Serial.println("Diode Output Sweep");
    printUnits();
    doDiodeOutputSweep();
    Serial.write(prompt);

  } else if (flags & SWEEP_FET_INPUT_DATA ||
             flags & SWEEP_FET_VOLTAGE_INPUT_DATA || flags & SWEEP_FET_R_DATA) {
    Serial.println("FET Input Sweep");
    printUnits();
    doNPNInputSweep();
    Serial.write(prompt);

  } else if (flags & SWEEP_FET_OUTPUT_DATA) {
    Serial.println("FET Output Sweep");
    printUnits();
    doOutputFETSweep();
    Serial.write(prompt);

  } else if (flags & SWEEP_FET_K_DATA || flags & SWEEP_FET_GM_DATA) {
    Serial.println("FET K/GM Sweep");
    printUnits();
    doFETKSweep();
    Serial.write(prompt);
  }
}

void printUnits(void) {
  if (pfs->units == 'M' || pfs->units == 'M') {
    Serial.println("Units: m");
  } else if (pfs->units == 'U' || pfs->units == 'U') {
    Serial.println("Units: u");
  }
}

void waitForSerial(void) {
  while (Serial.available()) {
    Serial.read();
  }
  while (!Serial.available()) {
  }
  Serial.println(Serial.read());
}

void Reset(void) {
  resetPins();
  resetVoltages();

  Serial.println(BANNER);
  resetSerial();
  Serial.write(prompt);
}

// Place program specific content here
void executeSerial(char *str) {
  // num defined the actual number of entries process from the serial buffer
  // i is a generic counter
  float vtemp, ctemp;

  // This function called when serial input in present in the serial buffer
  // The serial buffer is parsed and characters and numbers are scraped and
  // entered in the commands[] and numbers[] variables.
  parseSerial(str);

  // Process the commands
  // Note: Whenever a parameter is stated as [CLK] the square brackets are not
  // entered. The square brackets means that this is a command line parameter
  // entered after the command. E.g. F [n] [m] would be mean "F 0 7000000" is
  // entered (no square brackets entered)
  switch (commands[0]) {
      ////////////////////////////////////////////////////////////
      //////// CALIBRATION
      ////////////////////////////////////////////////////////////
    case 'C':                    // Calibrate
      if (commands[1] == 'R') {  // read raw ADC values
        if (flags & PRINT_ADC_VALUES) {
          flags &= ~PRINT_ADC_VALUES;
        } else {
          flags &= ~EXPORT_DATA;
          flags |= PRINT_ADC_VALUES;
        }
      } else if (commands[1] == 'M') {
        if (!checkResistors('A', 1)) {
          break;
        }
        getMaxVoltages();
      } else if (commands[1] == 'D') {
        setVoltage(pts->VbOutPin, CALIBRATION_VB_VOLTAGE);
        setVoltage(pts->VccOutPin, CALIBRATION_VC_VOLTAGE);
        for (i = 0; i < 10; i++) {
          getADCValues();
        }
        resetPins();
      } else {
        printConfig(PRINT_ALL_VALUES);
      }
      break;

      ////////////////////////////////////////////////////////////
      //////// Set defaults
      ////////////////////////////////////////////////////////////
    case 'D':  // Defaults
      setDefaultLimits();
      pts->rcresistor = RC_RESISTORHIGH;
      pts->rbresistor = RB_RESISTORLOW;
      checkResistors('A', 1);
      break;

      ////////////////////////////////////////////////////////////
      //////// Set defaults
      ////////////////////////////////////////////////////////////
    case 'E':                    // EEPROM
      if (commands[1] == 'R') {  // Read EEPROM ADC values
        EEPROM.get(addr, *pfs);

      } else if (commands[1] == 'I') {  // Initialize EEPROM ADC values{
        Serial.println("EE INIT");
        setDefaultLimits();
        pts->rcresistor = RC_RESISTORHIGH;
        pts->rbresistor = RB_RESISTORLOW;
        getMaxVoltages();
        EEPROM.put(addr, *pfs);

        delay(500);  // Allow time for write to complete
        EEPROM.get(addr, *pfs);
        if (pfs->version == EEPROM_HEADER) {
          printConfig(PRINT_ALL_VALUES);
        } else {
          Serial.println("EE Err");
        }

      } else if (commands[1] == 'W') {  // Initialize EEPROM ADC values{
        if (!checkResistors('A', 1) || !checkSweepRange()) {
          Serial.println("Cal Err");
          break;
        }

        EEPROM.put(addr, *pfs);
        delay(500);  // give delay to allow to complete
        EEPROM.get(addr, *pfs);
      }
      if (pfs->version == EEPROM_HEADER) {
        printConfig(PRINT_ALL_VALUES);
      } else {
        Serial.println("EE Err");
      }
      break;

      ////////////////////////////////////////////////////////////
      //////// Help
      ////////////////////////////////////////////////////////////
    case 'H':  // Help
      showHelp(QUIET);
      break;

      ////////////////////////////////////////////////////////////
      //////// Measure Voltages
      ////////////////////////////////////////////////////////////
    case 'M':                    // Measure voltages
      if (commands[1] == 'R') {  // read raw values
        Serial.println("RAW: ");
        getADCValues();

      } else {
        Serial.println("NPN: ");
        measureVoltages();
        displayVoltages();
      }
      break;

      ////////////////////////////////////////////////////////////
      //////// PWM Output
      ////////////////////////////////////////////////////////////
    case 'P':  // SET PWM
      if ((numbers[0] != (unsigned long)pts->VbOutPin &&
           numbers[0] != (unsigned long)pts->VccOutPin) ||
          numbers[1] > 255) {
        Serial.println("Num Err");
        break;
      }
      Serial.print("Pin ");
      Serial.print(numbers[0]);
      Serial.print(" PWM ");
      Serial.println(numbers[1]);
      analogWrite((int)numbers[0], (int)numbers[1]);
      break;

      ////////////////////////////////////////////////////////////
      //////// Reset
      ////////////////////////////////////////////////////////////
    case 'R':  // Reset
      Reset();
      break;

      ////////////////////////////////////////////////////////////
      //////// Set Parameters
      ////////////////////////////////////////////////////////////
    case 'S':  // Set current or voltage

      /*
      =============================================================
      Set Output Base current on pin numbers[0]
      =============================================================
      */
      if (commands[1] == 'I') {  // set Vb current
        if (!checkResistors('A', 1)) {
          break;
        }
        if (numbers[0] > (unsigned long)((float)MAX_VC_VOLTAGE *
                                         (float)1000000 / pts->rbresistor)) {
          Serial.print("uA < ");
          Serial.println((unsigned long)((float)MAX_VC_VOLTAGE *
                                         (float)1000000 / pts->rbresistor));
          break;
        }
        Serial.print("uA: ");
        Serial.println(numbers[1]);
        setVbCurrent((float)numbers[1] / (float)1000000);
        displayVoltages();
        break;
        /*
        =============================================================
        Set Output Voltage on Vb or Vcc pin
        =============================================================
        */
      } else if (commands[1] == 'V') {
        if (!checkResistors('A', 1)) {
          break;
        }
        if (pts->vbmax == 0 || pts->vcmax == 0) {
          Serial.println("Cal Err");
          break;
        }
        temp = 0;
        ctemp = vtemp = 0;
        vtemp = (float)numbers[0] / (float)1000;
        if (commands[2] == 'B') {
          temp = pts->VbOutPin;
          ctemp = pts->vbmax * 1000;
          if (commands[3] == 'O') {
            if (pts->VbCalibration == 0) {
              Serial.println("Cal Err");
              break;
            } else if (numbers[0] >
                           (unsigned long)(ctemp * pts->VbCalibration) ||
                       numbers[0] == 0) {
              Serial.print("mV <");
              Serial.println((unsigned long)(ctemp * pts->VbCalibration));
              break;
            }
            vtemp /= pts->VbCalibration;
          } else {
            if (numbers[0] > (unsigned long)ctemp || numbers[0] == 0) {
              Serial.print("mV <");
              Serial.println((unsigned long)ctemp);
              break;
            }
          }

        } else if (commands[2] == 'C') {
          temp = pts->VccOutPin;
          ctemp = pts->vcmax * 1000;
          if (commands[3] == 'O') {
            if (pts->VcCalibration == 0) {
              Serial.println("Cal Err");
              break;
            } else if (numbers[0] >
                           (unsigned long)(ctemp * pts->VcCalibration) ||
                       numbers[0] == 0) {
              Serial.print("mV <");
              Serial.println((unsigned long)(ctemp * pts->VcCalibration));
              break;
            }
            vtemp /= pts->VcCalibration;
          } else {
            if (numbers[0] > (unsigned long)ctemp || numbers[0] == 0) {
              Serial.print("mV <");
              Serial.println((unsigned long)ctemp);
              break;
            }
          }
        } else {
          showHelp(PUKE);
          break;
        }

        Serial.print("Pin ");
        Serial.print(temp);
        Serial.print(" mV ");
        Serial.println(vtemp, 1);
        Serial.println(" mV");

        setVoltage(temp, vtemp);
        delay(PWM_SETTLE_TIME);  // First voltage take a while to charge
                                 // capacitor.  Change from 30 to 40ms
        measureVoltages();
        displayVoltages();
        break;

        /*
        =============================================================
        Set Resistor values for Base & Collector
        =============================================================
        */
      } else if (commands[1] == 'R') {  // Set Resistor

        ////// Collector resistor
        if (commands[2] == 'C') {
          if (numbers[0] != RC_RESISTORLOW && numbers[0] != RC_RESISTORHIGH) {
            Serial.print("R ");
            Serial.print(RC_RESISTORLOW);
            Serial.print("|");
            Serial.print(RC_RESISTORHIGH);
            Serial.print(" Err");
            break;
          }
          pts->rcresistor = (int)numbers[0];
          printResistorValues(PRINT_COLLECTOR_VALUES);
          break;
          ////// Base resistor
        } else if (commands[2] == 'B') {
          if (numbers[0] != RB_RESISTORLOW && numbers[0] != RB_RESISTORHIGH) {
            Serial.print("R ");
            Serial.print(RB_RESISTORLOW);
            Serial.print("|");
            Serial.print(RB_RESISTORHIGH);
            Serial.print(" Err");
            break;
          }
          pts->rbresistor = (int)numbers[0];
          printResistorValues(PRINT_BASE_VALUES);
          break;
        } else {
          printResistorValues(PRINT_ALL_VALUES);
        }
        /*
        =============================================================
        set default Units
        =============================================================
        */
      } else if (commands[1] == 'U') {  // set default Units
        if (commands[2] != 'M' && commands[2] != 'U' && commands[2] != 'D') {
          Serial.print("Units: ");
          Serial.println(pts->units);
          Serial.print("U U|M|D");
          break;
        }
        pts->units = pfs->units = commands[2];
        Serial.print("Units: ");
        Serial.println(pts->units);
        break;

        /*
        =============================================================
        set data points in sweep
        =============================================================
        */
      } else if (commands[1] == 'P') {  // Set Sweep increment
                                        ////// Base points
        if (commands[2] == 'B') {
          if (numbers[0] < MIN_NUM_VB_VOLTAGE ||
              numbers[0] > MAX_NUM_VB_VOLTAGE) {
            Serial.print("P >");
            Serial.print(MIN_NUM_VB_VOLTAGE);
            Serial.print(" & <");
            Serial.print(MAX_NUM_VB_VOLTAGE);
            Serial.print(" Err");
            break;
          }
          pts->nvb = (int)numbers[0];
          Serial.print("B/G: ");
          Serial.println(pts->nvb);
          pts->vbinc = (pts->vbmax - pts->vbmin) / (float)pts->nvb;
          break;
          ////// Collector points
        } else if (commands[2] == 'C') {
          if (numbers[0] < MIN_NUM_VC_VOLTAGE ||
              numbers[0] > MAX_NUM_VC_VOLTAGE) {
            Serial.print("P >");
            Serial.print(MIN_NUM_VC_VOLTAGE);
            Serial.print(" & <");
            Serial.print(MAX_NUM_VC_VOLTAGE);
            Serial.print(" Err");
            break;
          }
          pts->nvc = (int)numbers[0];
          Serial.print("C/D: ");
          Serial.println(pts->nvc);
          pts->vcinc = (pts->vcmax - pts->vcmin) / (float)pts->nvc;
          break;
        } else {
          Serial.print("B/G: ");
          Serial.print(pts->nvb);
          Serial.print(" C/D: ");
          Serial.println(pts->nvc);
        }
        /*
        =============================================================
        set default units for sweep values
        =============================================================
        */
      } else if (commands[1] == 'U') {  // set default Units
        if (commands[2] != 'M' && commands[2] != 'U' && commands[2] != 'D') {
          Serial.print("Units: ");
          Serial.println(pts->units);
          break;
        }
        pts->units = pfs->units = commands[2];
        Serial.print("Units: ");
        Serial.println(pts->units);
        break;
        /*
        =============================================================
        set min max Vb and Vg voltages to sweep
        =============================================================
        */
      } else if (commands[1] ==
                 'M') {  // set min max Vb and Vg voltages to sweep
                         ////// Base Voltage
        if (commands[2] == 'B') {
          if (numbers[0] >= numbers[1] ||
              numbers[0] > (unsigned long)(MAX_VB_VOLTAGE * 1000) ||
              numbers[1] > (unsigned long)(MAX_VB_VOLTAGE * 1000) ||
              numbers[0] < (unsigned long)(MIN_VB_VOLTAGE * 1000) ||
              numbers[1] < (unsigned long)(MIN_VB_VOLTAGE * 1000) ||
              numbers[1] == 0) {
            Serial.println("Num Err");
            break;
          }
          pts->vbmin = pfs->vgmin = ((float)numbers[0]) / 1000.0;
          pts->vbmax = pfs->vgmax = ((float)numbers[1]) / 1000.0;
          pts->vbinc = (pts->vbmax - pts->vbmin) / (float)pts->nvb;
          pfs->vginc = (pfs->vgmax - pfs->vgmin) / (float)pfs->nvg;
          printSweepValues(PRINT_BASE_VALUES);
          ////// Collector Voltage
        } else if (commands[2] == 'C') {
          if (numbers[0] >= numbers[1] ||
              numbers[0] > (unsigned long)(MAX_VC_VOLTAGE * 1000) ||
              numbers[1] > (unsigned long)(MAX_VC_VOLTAGE * 1000) ||
              numbers[0] < (unsigned long)(MIN_VC_VOLTAGE * 1000) ||
              numbers[1] < (unsigned long)(MIN_VC_VOLTAGE * 1000) ||
              numbers[1] == 0) {
            Serial.println("Num Err");
            break;
          }
          pts->vcmin = pfs->vdmin = ((float)numbers[0]) / 1000.0;
          pts->vcmax = pfs->vdmax = ((float)numbers[1]) / 1000.0;
          pts->vcinc = (pts->vcmax - pts->vcmin) / (float)pts->nvc;
          printSweepValues(PRINT_COLLECTOR_VALUES);
        } else {
          printSweepValues(PRINT_ALL_VALUES);
        }
        break;
      } else {
        showHelp(PUKE);
        break;
      }
      break;

      ////////////////////////////////////////////////////////////
      //////// Trace
      ////////////////////////////////////////////////////////////
    case 'T':           // Trace
      pts->header = 0;  // set to show trace header

      /*
      =============================================================
      NPN BJT Traces
      =============================================================
      */
      if ((!checkResistors('A', 1)) || !checkSweepRange()) {
        break;
      }

      if (commands[1] == 'N') {
        flags = flags | EXPORT_DATA;
        if (commands[2] == 'I') {  // Input sweep by VCE or base voltage
          if (commands[3] == 'V') {
            flags = flags | SWEEP_BJT_INPUT_DATA_BY_VCE;
          } else {
            flags = flags | SWEEP_BJT_INPUT_DATA;
          }
        } else if (commands[2] == 'O') {  // Output Sweep
          flags = flags | SWEEP_BJT_OUTPUT_DATA;
        } else if (commands[2] == 'B') {
          if (commands[3] == 'C') {
            flags = flags | SWEEP_BJT_BETAC_DATA;
          } else {
            flags = flags | SWEEP_BJT_BETA_DATA;
          }
        } else if (commands[2] == 'R') {  // Re Sweep
          flags = flags | SWEEP_BJT_RE_DATA;
        } else if (commands[2] == 'G') {  // Output Sweep
          flags = flags | SWEEP_BJT_GM_DATA;

        } else {
          clearSweepFlags();
          showHelp(PUKE);
        }

        /*
        =============================================================
        Diode Traces
        =============================================================
        */
      } else if (commands[1] == 'D') {
        flags = flags | EXPORT_DATA;
        if (commands[2] == 'I') {
          flags = flags | SWEEP_DIODE_INPUT_DATA;
        } else if (commands[2] == 'O') {
          flags = flags | SWEEP_DIODE_OUTPUT_DATA;
        } else {
          clearSweepFlags();
          showHelp(PUKE);
        }

        /*
        =============================================================
        NMOS FET Traces
        =============================================================
        */
      } else if (commands[1] == 'F') {
        flags = flags | EXPORT_DATA;
        if (commands[2] == 'I') {
          if (commands[3] == 'V') {
            flags = flags | SWEEP_FET_VOLTAGE_INPUT_DATA;
          } else {
            flags = flags | SWEEP_FET_INPUT_DATA;
          }
        } else if (commands[2] == 'O') {
          flags = flags | SWEEP_FET_OUTPUT_DATA;
        } else if (commands[2] == 'K') {
          flags = flags | SWEEP_FET_K_DATA;
        } else if (commands[2] == 'G') {
          flags = flags | SWEEP_FET_GM_DATA;
        } else if (commands[2] == 'R') {
          flags = flags | SWEEP_FET_R_DATA;
        } else {
          clearSweepFlags();
          showHelp(PUKE);
        }

      } else {
        clearSweepFlags();
        showHelp(PUKE);
      }
      break;

    // If an undefined command is entered, display an error message
    default:
      errorOut();
  }
}

void showHelp(char puke) {
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
  Serial.println("S [V] [B|C] [O] [mV]");  // Set voltage on base or Vcc pin.  Set output voltage delivered to DUT
  Serial.println("S [I] [uA]");        // Set current om base
  Serial.println("S [U] [D|M|U]");     // Set default unit to decimal, Milla
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
  Serial.println("T [DI|DO]");               // Diode input, Diode output using
                                             // resistor R break;
}
