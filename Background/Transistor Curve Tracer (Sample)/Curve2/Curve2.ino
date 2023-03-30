/*

Curve Tracer

ver 1.0

*/

#include <Arduino.h>

#include "SimpleILI9341.h"

#include <avr/pgmspace.h>

#include <SPI.h>

int val = 0;
byte i = 0;
int prev_x = 0, prev_y = 0;
bool ExecSerialTx = false; // send scans to PC
bool SendAdcValues = false; // send ADC values to PC
int minYposGain, maxYposGain, minBaseGain, maxBaseGain; // used when calc gain

const int TFT_WID = 320;
const int TFT_HGT = 240;
const int ADC_MAX = 1024;

const int pin_DAC_CS = 10;
const int pin_ADC_NPN_Vcc = 0;
const int pin_ADC_NPN_Vce = 1;
const int pin_ADC_PNP_Vce = 2;
const int pin_ADC_PNP_Vcc = 3;
const int pin_Adc_12V = 7;
const int pin_Adc_Bat = 6;

const int R1 = 33; // ADC input potential divider upper resistor k-ohms
const int R2 = 47; // ADC input potential divider lower resistor k-ohms
const int R3 = 100; // collector resistor ohms
const int R4 = 68; // DAC op amp feedback upper resistor k-ohms
const int R5 = 33; // DAC op amp feedback lower resistor k-ohms
const int R6 = 10; // measure battery volts potential divider upper resistor k-ohms
const int R7 = 33; // measure battery volts potential divider lower resistor k-ohms

const int DacVref = 40; // DAC Vref in 100s of mV
const int AdcVref = 5; // ADC Vref in V
const int mAmax = 50; // Ic for top of screen
const int Ib_inc = 50; // increment for Ibase step
const unsigned long TimeoutPeriod = 60000;
int MinVgate = 0;
int MaxVgate = 12;
int MinIbase = 0;
int MaxIbase = 350;
long ngJFET = 255;

// be careful using these inside if statements:
#define SerialPrint(s) {if (ExecSerialTx) Serial.print(s);}
#define SerialPrintLn(s) {if (ExecSerialTx) Serial.println(s);}

enum TkindDUT {tkNothing, tkPNP, tkNPN, tkPMOSFET, tkNMOSFET, tkNJFET, tkPJFET, tkPDiode, tkNDiode};
TkindDUT curkind;

enum TclassDUT {tcBipolar, tcMOSFET, tcJFET};
TclassDUT CurDUTclass = tcBipolar;

const uint8_t bmpPNP[] PROGMEM = {
  21, 0, // width
  30, 0, // height
  0x3F, 0xFF, 0xF9, 0xFF, 0xFF, 0xCF, 0xFF, 0xFE, 0x7F, 0xFF, 0xF1, 0xFF, 0x3F, 0xC7, 0xF9, 0xFF,
  0x1F, 0xCF, 0xFC, 0x7E, 0x7F, 0xF1, 0xF3, 0xFF, 0xC7, 0x9F, 0xFF, 0x1C, 0xFF, 0xFC, 0x67, 0xFF,
  0xF1, 0x3F, 0xFF, 0xC1, 0xFF, 0xFF, 0x00, 0x1F, 0xFC, 0x00, 0xFF, 0x93, 0xFF, 0xF0, 0x9F, 0xFE,
  0x0C, 0xFF, 0xC0, 0x67, 0xFC, 0x07, 0x3F, 0xF0, 0x39, 0xFF, 0x83, 0xCF, 0xF8, 0x1E, 0x7F, 0x8D,
  0xF3, 0xF8, 0xFF, 0x9F, 0xCF, 0xFF, 0xFE, 0x7F, 0xFF, 0xF3, 0xFF, 0xFF, 0x9F, 0xFF, 0xFC
};

const uint8_t bmpNPN[] PROGMEM = {
  21, 0, // width
  30, 0, // height
  0xFF, 0xFF, 0xE7, 0xFF, 0xFF, 0x3F, 0xFF, 0xF9, 0xFF, 0xFF, 0xCF, 0xE7, 0xFC, 0x7F, 0x3F, 0xC7,
  0xF9, 0xFC, 0x7F, 0xCF, 0xC7, 0xFE, 0x7C, 0x7F, 0xF3, 0xC7, 0xFF, 0x9C, 0x7F, 0xFC, 0xC7, 0xFF,
  0xE4, 0x7F, 0xFF, 0x07, 0xFC, 0x00, 0x7F, 0xE0, 0x03, 0xFF, 0xFE, 0x0F, 0xFF, 0xF2, 0x3B, 0xFF,
  0x98, 0x9F, 0xFC, 0xE0, 0x7F, 0xE7, 0x83, 0xFF, 0x38, 0x0F, 0xF9, 0x80, 0x7F, 0xCF, 0x01, 0xFE,
  0x7E, 0x0F, 0xF3, 0xFC, 0x3F, 0xFF, 0xF9, 0xFF, 0xFF, 0xCF, 0xFF, 0xFE, 0x7F, 0xFF, 0xF0
};

const uint8_t bmpNMOSFET[] PROGMEM = {
  23, 0, // width
  34, 0, // height
  0xFF, 0xFF, 0xF9, 0xFF, 0xFF, 0xF3, 0xFF, 0xFF, 0xE7, 0xFF, 0xFF, 0xCF, 0xFF, 0xFF, 0x9F, 0xFF,
  0xFF, 0x3F, 0xFF, 0xFE, 0x7F, 0xFF, 0xFC, 0xFE, 0x60, 0x01, 0xFC, 0xC0, 0x03, 0xF9, 0x9F, 0xFF,
  0xF3, 0x3F, 0xFF, 0xE7, 0xFF, 0xFF, 0xCF, 0xFB, 0xFF, 0x99, 0xC7, 0xFF, 0x32, 0x0F, 0xFE, 0x60,
  0x01, 0xFC, 0xC0, 0x03, 0xF9, 0x90, 0x67, 0xF3, 0x38, 0xCF, 0xE7, 0xFD, 0x9F, 0xCF, 0xFF, 0x3F,
  0x99, 0xFE, 0x7F, 0x33, 0xFC, 0x00, 0x60, 0x00, 0x00, 0xC0, 0x03, 0xFF, 0xFF, 0xE7, 0xFF, 0xFF,
  0xCF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0x3F, 0xFF, 0xFE, 0x7F, 0xFF, 0xFC, 0xFF, 0xFF, 0xF9, 0xFF,
  0xFF, 0xF0
};

const uint8_t bmpPMOSFET[] PROGMEM = {
  23, 0, // width
  34, 0, // height
  0x3F, 0xFF, 0xFE, 0x7F, 0xFF, 0xFC, 0xFF, 0xFF, 0xF9, 0xFF, 0xFF, 0xF3, 0xFF, 0xFF, 0xE7, 0xFF,
  0xFF, 0xCF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0x00, 0x0C, 0xFE, 0x00, 0x19, 0xFF, 0xFF, 0x33, 0xFF,
  0xFE, 0x67, 0xFF, 0xFF, 0xCF, 0xFF, 0xBF, 0x9F, 0xFC, 0x73, 0x3F, 0xE0, 0xE6, 0x7F, 0x80, 0x0C,
  0xFE, 0x00, 0x19, 0xFC, 0x07, 0x33, 0xF9, 0x8E, 0x67, 0xF3, 0xDF, 0xCF, 0xE7, 0xFF, 0x9F, 0xCF,
  0xF3, 0x3F, 0x9F, 0xE6, 0x7F, 0x00, 0x0C, 0x00, 0x00, 0x18, 0x00, 0xFF, 0xFF, 0xF9, 0xFF, 0xFF,
  0xF3, 0xFF, 0xFF, 0xE7, 0xFF, 0xFF, 0xCF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0x3F, 0xFF, 0xFE, 0x7F,
  0xFF, 0xFC
};

const uint8_t bmpNJFET[] PROGMEM = {
  21, 128, // width (run-length encoded)
  30, 0, // height
  0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x05, 0x02, 0x0C,
  0x02, 0x05, 0x02, 0x0C, 0x09, 0x0C, 0x09, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x06, 0x01, 0x0C, 0x02, 0x04,
  0x03, 0x0C, 0x02, 0x02, 0x05, 0x05, 0x2C, 0x05, 0x02, 0x02, 0x05, 0x05, 0x02, 0x05, 0x02, 0x04, 0x03, 0x05, 0x02, 0x0D,
  0x01, 0x05, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13
};

const uint8_t bmpPJFET[] PROGMEM = {
  21, 128, // width (run-length encoded)
  30, 0, // height
  0x00, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x0C, 0x02, 0x05,
  0x02, 0x0C, 0x02, 0x05, 0x02, 0x0C, 0x09, 0x0C, 0x09, 0x0C, 0x02, 0x13, 0x02, 0x13, 0x02, 0x0F, 0x01, 0x03, 0x02, 0x0D,
  0x03, 0x03, 0x02, 0x0B, 0x05, 0x03, 0x02, 0x07, 0x2A, 0x04, 0x05, 0x03, 0x02, 0x05, 0x02, 0x06, 0x03, 0x03, 0x02, 0x05,
  0x02, 0x08, 0x01, 0x0A, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02, 0x13, 0x02
};

const uint8_t bmpPDiodeBig[] PROGMEM = {
  20, 128, // width run-length encoded
  24, 0, // height
  0x00, 0x09, 0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x09, 0x14, 0x01, 0x12, 0x03, 0x10, 0x05,
  0x0E, 0x07, 0x0C, 0x09, 0x0A, 0x0B, 0x08, 0x0D, 0x06, 0x0F, 0x04, 0x11, 0x02, 0x09, 0x28, 0x09, 0x02, 0x12, 0x02, 0x12,
  0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x09
};

const uint8_t bmpNDiodeBig[] PROGMEM = {
  20, 128, // width run-length encoded
  24, 0, // height
  0x00, 0x09, 0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x09, 0x28, 0x09, 0x02, 0x11, 0x04, 0x0F,
  0x06, 0x0D, 0x08, 0x0B, 0x0A, 0x09, 0x0C, 0x07, 0x0E, 0x05, 0x10, 0x03, 0x12, 0x01, 0x14, 0x09, 0x02, 0x12, 0x02, 0x12,
  0x02, 0x12, 0x02, 0x12, 0x02, 0x12, 0x02, 0x09
};

const uint8_t bmpPDiodeSmall[] PROGMEM = {
  15, 0, // width
  28, 0, // height
  0xFE, 0x01, 0xF8, 0x03, 0xF3, 0xFF, 0xE7, 0xFF, 0xCF, 0xFF, 0x9F, 0xFF, 0x3F, 0xFE, 0x7F, 0xFC,
  0xFE, 0x00, 0x06, 0x00, 0x1E, 0x00, 0x7E, 0x01, 0xFE, 0x07, 0xFE, 0x1F, 0xFE, 0x7F, 0x00, 0x02,
  0x00, 0x07, 0xF3, 0xFF, 0xE7, 0xFF, 0xCF, 0xFF, 0x9F, 0xFF, 0x3F, 0xFE, 0x7F, 0xFC, 0xFF, 0xF9,
  0xFF, 0xF0, 0x07, 0xF0, 0x00
};

const uint8_t bmpNDiodeSmall[] PROGMEM = {
  15, 0, // width
  28, 0, // height
  0x00, 0xFE, 0x00, 0xFF, 0xF9, 0xFF, 0xF3, 0xFF, 0xE7, 0xFF, 0xCF, 0xFF, 0x9F, 0xFF, 0x3F, 0xFE,
  0x7F, 0xFC, 0xFE, 0x00, 0x04, 0x00, 0x0F, 0xE7, 0xFF, 0x87, 0xFE, 0x07, 0xF8, 0x07, 0xE0, 0x07,
  0x80, 0x06, 0x00, 0x07, 0xF3, 0xFF, 0xE7, 0xFF, 0xCF, 0xFF, 0x9F, 0xFF, 0x3F, 0xFE, 0x7F, 0xFC,
  0xFC, 0x01, 0xF8, 0x07, 0xF0
};

const uint8_t bmpUpDownArrow[] PROGMEM = {
  24, 0, // width
  14, 0, // height
  0xFD, 0xFF, 0xFF, 0xF8, 0xFF, 0x1F, 0xF0, 0x7F, 0x1F, 0xE0, 0x3F, 0x1F, 0xC0, 0x1F, 0x1F, 0x80,
  0x0F, 0x1F, 0x00, 0x07, 0x1F, 0xF8, 0xE0, 0x00, 0xF8, 0xF0, 0x01, 0xF8, 0xF8, 0x03, 0xF8, 0xFC,
  0x07, 0xF8, 0xFE, 0x0F, 0xF8, 0xFF, 0x1F, 0xFF, 0xFF, 0xBF
};

//-------------------------------------------------------------------------
// DrawKindStr
//   draws the kin of the DUT at the top of the screen
//-------------------------------------------------------------------------

void DrawKindStr(TkindDUT kind) {
  switch (kind) {
    case tkPNP:
      DrawStringAt((TFT_WID - 39) / 2, 15, "PNP", LargeFont, TFT_YELLOW);
      DrawBitmapMono((TFT_WID - 28) / 2, 25, bmpPNP, TFT_YELLOW);
      break;
    case tkNPN:
      DrawStringAt((TFT_WID - 42) / 2, 15, "NPN", LargeFont, TFT_YELLOW);
      DrawBitmapMono((TFT_WID - 28) / 2, 25, bmpNPN, TFT_YELLOW);
      break;
    case tkPMOSFET:
      DrawStringAt((TFT_WID - 80) / 2, 15, "p-MOSFET", LargeFont, TFT_YELLOW);
      DrawBitmapMono((TFT_WID - 28) / 2, 25, bmpPMOSFET, TFT_YELLOW);
      break;
    case tkNMOSFET:
      DrawStringAt((TFT_WID - 80) / 2, 15, "n-MOSFET", LargeFont, TFT_YELLOW);
      DrawBitmapMono((TFT_WID - 28) / 2, 25, bmpNMOSFET, TFT_YELLOW);
      break;
    case tkPJFET:
      DrawStringAt((TFT_WID - 59) / 2, 15, "p-JFET", LargeFont, TFT_YELLOW);
      DrawBitmapMono((TFT_WID - 28) / 2, 25, bmpPJFET, TFT_YELLOW);
      break;
    case tkNJFET:
      DrawStringAt((TFT_WID - 59) / 2, 15, "n-JFET", LargeFont, TFT_YELLOW);
      DrawBitmapMono((TFT_WID - 28) / 2, 25, bmpNJFET, TFT_YELLOW);
      break;
    case tkPDiode:
    case tkNDiode:
      DrawStringAt((TFT_WID - 47) / 2, 15, "Diode", LargeFont, TFT_YELLOW);
      DrawBitmapMono((TFT_WID - 20) / 2, 25, bmpPDiodeBig, TFT_YELLOW);
      break;
    default: // unknown
      DrawStringAt((TFT_WID - 117) / 2, 15, "Curve Tracer", LargeFont, TFT_YELLOW);
  }
}

//-------------------------------------------------------------------------
// TurnOffLoad
//   sets load current to zero
//-------------------------------------------------------------------------

void TurnOffLoad(TkindDUT kind) {
  /*
    if (kind == tkNothing) {
    //    if (analogRead(pin_ADC_PNP_Vce)-analogRead(pin_ADC_PNP_Vcc) > analogRead(pin_ADC_NPN_Vcc)-analogRead(pin_ADC_NPN_Vce))
      if (analogRead(pin_ADC_PNP_Vce)-analogRead(pin_ADC_PNP_Vcc) > 50)
        kind = tkPNP;
      else
        kind = tkNPN;
    }
  */

  switch (kind) {
    case tkPMOSFET:
    case tkNJFET:
    case tkPNP:
    case tkPDiode:
      SetDacBase(0, 0);
      SetDacVcc(255, 0);
      break;
    case tkNPN:
    case tkNMOSFET:
    case tkPJFET:
    case tkNDiode:
      SetDacBase(255, 0);
      SetDacVcc(0, 0);
      break;
    default:
      SetDacBase(255, 0);
      SetDacVcc(255, 0);
  }
}

//-------------------------------------------------------------------------
// TestDeviceKind
//    is there a DUT inserted?
//    returns kind of device
//    (only knows transistor/diode/none and neg/pos - assumed rest from user's selection)
//    'kind' parameter is a hint as to what it might be
//-------------------------------------------------------------------------

TkindDUT TestDeviceKind(TkindDUT kind, bool TestForDiode) {
  int k, i;
  static bool b;

  b = !b;

  //Serial.print("K="); Serial.print(kind);
  if (kind == tkNothing && CurDUTclass == tcJFET)
    kind = b ? tkNJFET : tkPJFET;
  //Serial.println(kind);

  switch (kind) {
    case tkNJFET:   SetDacBase(255, 0);     SetDacVcc(0, 0); break;
    case tkPJFET:   SetDacBase(0, 0);       SetDacVcc(255, 0); break;
    default:        SetDacBase(128, 0);     SetDacVcc(128, 0);
  }

  for (k = 0; k <= 30; k++) {
    delay(1);

    switch (kind) {
      case tkPMOSFET:
      case tkNJFET:
      case tkPNP:
      case tkPDiode:
      case tkNothing:
        i = analogRead(pin_ADC_PNP_Vce) - analogRead(pin_ADC_PNP_Vcc);
        if (i > 50) {
          if (TestForDiode) {
            if (kind != tkNJFET) {
              SetDacBase(255, 30);
              if (i - (analogRead(pin_ADC_PNP_Vce) - analogRead(pin_ADC_PNP_Vcc)) < 50)
                kind = tkPDiode;
            }
          }
          TurnOffLoad(tkPNP);
          if (kind == tkNothing) {
            switch (CurDUTclass) {
              case tcMOSFET: return tkPMOSFET;
              case tcJFET:   return tkNJFET;
              default:       return tkPNP;
            }
          }
          return kind;
        }
    }

    switch (kind) {
      case tkNPN:
      case tkNMOSFET:
      case tkPJFET:
      case tkNDiode:
      case tkNothing:
        i = analogRead(pin_ADC_NPN_Vcc) - analogRead(pin_ADC_NPN_Vce);
        if (i > 50) {
          if (TestForDiode) {
            if (kind != tkNJFET) {
              SetDacBase(0, 30);
              if (i - (analogRead(pin_ADC_NPN_Vcc) - analogRead(pin_ADC_NPN_Vce)) < 50)
                kind = tkNDiode;
            }
          }
          TurnOffLoad(tkNPN);
          if (kind == tkNothing) {
            switch (CurDUTclass) {
              case tcMOSFET: return tkNMOSFET;
              case tcJFET:   return tkPJFET;
              default:       return tkNPN;
            }
          }
          return kind;
        }
    }
  }
  return tkNothing;
}

//-------------------------------------------------------------------------
// GetSerial
//   waits for and gets a serial input char
//-------------------------------------------------------------------------

uint8_t GetSerial() {
  while (Serial.available() == 0) {};
  return Serial.read();
}

//-------------------------------------------------------------------------
// SetDac
//   sets either of the DACs
//   x001 nnnn nnnn 0000
//-------------------------------------------------------------------------

void SetDac(uint8_t value, uint8_t cmd) {
  SPI.beginTransaction(SPISettings(4000000UL, MSBFIRST, SPI_MODE0));

  digitalWrite(pin_DAC_CS, LOW);
  SPI.transfer((value >> 4) | cmd);
  SPI.transfer(value << 4);
  digitalWrite(pin_DAC_CS, HIGH);

  SPI.endTransaction();
}

//-------------------------------------------------------------------------
// SetDacVcc
//   sets the collector DAC output
//-------------------------------------------------------------------------

void SetDacVcc(uint8_t value, int tDelay) {
  SetDac(value, 0x90);
  if (tDelay > 0)
    delay(tDelay);
}

//-------------------------------------------------------------------------
// SetDacBase
//   sets the base DAC output
//-------------------------------------------------------------------------

void SetDacBase(uint8_t value, int tDelay) {
  SetDac(value, 0x10);
  if (tDelay > 0)
    delay(tDelay);
}

//-------------------------------------------------------------------------
// InitGraph
//   draws the grid background of the graph
//-------------------------------------------------------------------------

void InitGraph(TkindDUT kind) {
  long ix, x, iy, y;

  ClearDisplay(TFT_BLACK);

  DrawStringAt(2, TFT_HGT - 4, "0", SmallFont, TFT_DARKGREY);
  DrawLine(0, 0, 0, TFT_HGT, TFT_DARKGREY);
  DrawLine(0, TFT_HGT - 1, TFT_WID, TFT_HGT - 1, TFT_DARKGREY);

  for (ix = 1; ix <= 12; ix++) {
    x = TFT_WID * ix * R1 / (R2 + R1) / AdcVref;
    ILI9341SetCursor(x + 2, TFT_HGT - 3);
    if (kind == tkPNP || kind == tkPMOSFET || kind == tkPJFET)
      DrawString("-", SmallFont, TFT_DARKGREY);
    DrawInt(ix, SmallFont, TFT_DARKGREY);
    DrawString("V", SmallFont, TFT_DARKGREY);
    DrawLine(x, 0, x, TFT_HGT, TFT_DARKGREY);
  }

  for (iy = 5; iy <= mAmax; iy += 5) {
    y = TFT_HGT - 1 - TFT_HGT * iy / mAmax;
    ILI9341SetCursor(2, y + 8);
    if (ix > 0 && (kind == tkPNP || kind == tkPMOSFET || kind == tkPJFET))
      DrawString("-", SmallFont, TFT_DARKGREY);
    DrawInt(iy, SmallFont, TFT_DARKGREY);
    DrawString("mA", SmallFont, TFT_DARKGREY);
    DrawLine(0, y, TFT_WID, y, TFT_DARKGREY);
  }

  DrawKindStr(kind);
}

//-------------------------------------------------------------------------
// Graph
//   draws one step of the curve
//   Vcc, Vce in ADC counts
//   base in uA
//-------------------------------------------------------------------------

void Graph(bool isMove, bool isNPN, int Vcc, int Vce, int base, int Adc_12V) {
  long i, j;
  static int px, py;

  if (isNPN) {
    i = Vce;
    j = Vcc - Vce;
  } else {
    i = Adc_12V - Vce;
    j = Vce - Vcc;
  }

  if (isMove)
  {
    SerialPrint("m ");
  } else {
    SerialPrint("l ");
  }

  SerialPrint(i * 1000 * (R2 + R1)*AdcVref / ADC_MAX / R1); SerialPrint(","); // print Vce in mV

  i = TFT_WID * i / ADC_MAX;
  j = j * (R2 + R1) * 48000 / R3 / R1 / ADC_MAX; // convert j to 100s of uA
  SerialPrint(j); SerialPrint(",");
  j = TFT_HGT - 1 - TFT_HGT * j / (mAmax * 10);
  if (j > TFT_HGT - 1) j = TFT_HGT - 1;

  SerialPrintLn(base * 5);

  /*
    if (isMove) {
      px = i*3;
      py = j*3;
    } else {
      px = (px*2)/3+i;
      py = (py*2)/3+j;
      i = px/3;
      j = py/3;
    }
  */

  if (isMove) {
    px = i * 2;
    py = j * 2;
  } else {
    px = px / 2 + i;
    py = py / 2 + j;
    i = px / 2;
    j = py / 2;
  }

  if (!isMove) {
    DrawLine(prev_x, prev_y, i, j, TFT_WHITE);

    if ((prev_x <= TFT_WID / 4) and (i > TFT_WID / 4) and (j < TFT_HGT - TFT_HGT / 20)) {
      if (maxYposGain < 0) {
        minYposGain = j;
        minBaseGain = base;
      }
      maxYposGain = j;
      maxBaseGain = base;
    }
  }

  prev_x = i;
  prev_y = j;
}

//-------------------------------------------------------------------------
// GetPMosfetThreshold
//   calc threshold V of MOSFET ; result in 100s of mV
//-------------------------------------------------------------------------

int GetPMosfetThreshold() {
  int gate;
  SetDacVcc(128, 1);
  for (gate = 0; gate <= 100; gate++) {
    SetDacBase(220 - gate * 21 / 10, 10);
    if (GetAdcSmooth(pin_ADC_PNP_Vce) - GetAdcSmooth(pin_ADC_PNP_Vcc) > 10)
      return gate;
  }
  return 0;
}

//-------------------------------------------------------------------------
// GetMosfetThreshold
//   calc threshold V of MOSFET ; result in 100s of mV
//-------------------------------------------------------------------------

int GetNMosfetThreshold() {
  int gate;
  SetDacVcc(128, 1);
  for (gate = 0; gate <= 100; gate++) {
    SetDacBase(gate * 21 / 10, 10);
    if (GetAdcSmooth(pin_ADC_NPN_Vcc) - GetAdcSmooth(pin_ADC_NPN_Vce) > 10)
      return gate;
  }
  return 0;
}

//-------------------------------------------------------------------------
// GetJfetPinchOff
//   calc pinch-off V of JFET  ; result in 100s of mV
//-------------------------------------------------------------------------

int GetJfetPinchOff(TkindDUT kind) {
  int gate;
  for (gate = 0; gate <= 100; gate++) {
    if (SetJFETvolts(kind, 128, gate)) {
      if (kind = tkNJFET) {
        if (GetAdcSmooth(pin_ADC_PNP_Vce) - GetAdcSmooth(pin_ADC_PNP_Vcc) <= 0)
          return gate;
      } else {
        if (GetAdcSmooth(pin_ADC_NPN_Vcc) - GetAdcSmooth(pin_ADC_NPN_Vce) > 10)
          return gate;
      }
    }
  }
  return 0;
}

//-------------------------------------------------------------------------
// EndScan
//   at end of drawing curves - calculates gain
//-------------------------------------------------------------------------

void EndScan(TkindDUT kind) {
  int i = 0;
  TurnOffLoad(kind);

  switch (kind) {
    case tkPNP:
    case tkNPN:
      if (maxBaseGain > minBaseGain) {
        DrawKindStr(kind);
        DrawString(" gain=", LargeFont, TFT_CYAN);
        i = long(minYposGain - maxYposGain) * mAmax * 1000 / (maxBaseGain - minBaseGain) / TFT_HGT / 5;
        DrawInt(i, LargeFont, TFT_CYAN);
      } else {
        SerialPrintLn("g 0");
        return;
      }
      break;

    case tkNMOSFET:
      DrawKindStr(kind);
      DrawString(" Vth=", LargeFont, TFT_CYAN);
      i = GetNMosfetThreshold();
      DrawDecimal(i, LargeFont, TFT_CYAN);
      break;

    case tkPMOSFET:
      DrawKindStr(kind);
      DrawString(" Vth=", LargeFont, TFT_CYAN);
      i = GetPMosfetThreshold();
      DrawDecimal(i, LargeFont, TFT_CYAN);
      break;

    case tkPJFET:
    case tkNJFET:
      DrawKindStr(kind);
      DrawString(" Voff=", LargeFont, TFT_CYAN);
      if (kind == tkNJFET)
        DrawString("-", LargeFont, TFT_CYAN);
      i = GetJfetPinchOff(kind);
      DrawDecimal(i, LargeFont, TFT_CYAN);
      break;
  }

  SerialPrint("g "); SerialPrintLn(i);
}

//-------------------------------------------------------------------------
// MedianOfFive
//   returns the middle value of 5 values
//-------------------------------------------------------------------------

int MedianOfFive(int a, int b, int c, int d, int e) {
  int tmp;
  if (b < a) {
    tmp = a;
    a = b;
    b = tmp;
  }
  if (d < c) {
    tmp = c;
    c = d;
    d = tmp;
  }
  if (c < a) {
    tmp = b;
    b = d;
    d = tmp;
    c = a;
  }
  if (b < e) {
    tmp = e;
    e = b;
    b = tmp;
  }
  if (e < c) {
    tmp = b;
    b = d;
    d = tmp;
    e = c;
  }
  if (d < e) return d; else return e;
}

//-------------------------------------------------------------------------
// GetAdcSmooth
//   mean of N readings of ADC
//-------------------------------------------------------------------------

int GetAdcSmooth(int pin) {
  int i, sum;
  const int n = 10;
  sum = 0;
  for (i = 1; i <= n; i++) {
    sum += analogRead(pin);
    delayMicroseconds(10);
  }
  return sum / n;
}

//-------------------------------------------------------------------------
// GetAdcSmooth
//   median of N readings of ADC
//-------------------------------------------------------------------------

int xGetAdcSmooth(int pin) {
  int i, a[5];
  for (i = 0; i <= 4; i++) {
    a[i] = analogRead(pin);
    delayMicroseconds(30);
  }
  return MedianOfFive(a[0], a[1], a[2], a[3], a[4]);
}

//-------------------------------------------------------------------------
// DrawDecimal
//   draw i/10 with one d.p.
//-------------------------------------------------------------------------

void DrawDecimal(int i, word Font, uint16_t color) {
  if (i < 0) {
    DrawString("-", Font, color);
    i = -i;
  }
  DrawInt(i / 10, Font, color);
  DrawString(".", Font, color);
  DrawInt(i % 10, Font, color);
}

//-------------------------------------------------------------------------
// HaveTouch
//   is the user touching the screen anywhere
//-------------------------------------------------------------------------

bool HaveTouch() {
  int x, y;
  return GetTouch(&x, &y);
}

//-------------------------------------------------------------------------
// SetJFETvolts
//   Set Vcc and Vgs for an n-JFET (Vcc measured in DAC counts; Vgs measured in 100s of mV)
//-------------------------------------------------------------------------

bool SetJFETvolts(TkindDUT kind, int Vcc, int Vgs) {
  int ADCs, i;
  int p1 = -1, p2 = -1, p3 = -1;

  SetDacVcc(Vcc, 1);

  do {
    SetDacBase(ngJFET, 0);
    ADCs = GetAdcSmooth(pin_ADC_PNP_Vce);
    const long c1 = long(1000) * AdcVref * 10 * (R2 + R1) / R1 / ADC_MAX;
    const long c2 = long(1000) * DacVref * (R4 + R5) / R5 / 256;
    const long c3n = 28; // fudge factor
    const long c3p = 0; // fudge factor

    if (kind == tkNJFET)
      ngJFET = (ADCs * c1 - (long(Vgs) * 1000 + Vcc * c3n)) / c2;
    else
      ngJFET = ((long(Vgs) * 1000 + Vcc * c3p) - ADCs * c1) / c2;

    if (ngJFET < 0) ngJFET = 0;
    if (ngJFET > 255) ngJFET = 255;

    p1 = p2;
    p2 = p3;
    p3 = ngJFET;
  } while (p1 != p3);

  return (ngJFET > 0) && (ngJFET < 255);
}

//-------------------------------------------------------------------------
// ScanAllNeg
//   draw curves for a component on the PNP side of the DUT socket
//     due to non-linearities, iFirst is different
//     Dac Base = iFirst
//            iConst+(1*iInc) / 10
//            iConst+(2*iInc) / 10
//            iConst+(3*iInc) / 10
//   if user touches the screen at the end of a line, the scan is terminated and the function returns true
//-------------------------------------------------------------------------

void ScanAllNeg(TkindDUT kind, int iFirst, int iConst, int iInc, int minBase, int maxBase, int incBase, int Adc_12V) {
  int DacVcc; // collector
  int base;

  DacVcc = 255;
  maxYposGain = -1;
  minBaseGain = 0;
  maxBaseGain = 0;

  SerialPrintLn("n");

  for (base = minBase; base <= maxBase; base += incBase) {
    if (base == 0)
    {
      SetDacBase(iFirst, 0);
    } else {
      i = iConst + base * iInc / 10;
      if (i < 0) break;
      SetDacBase(i, 0);
    }

    for (DacVcc = 255; DacVcc >= 0; DacVcc -= 2) {
      SetDacVcc(DacVcc, 1);
      if (DacVcc == 255)
        delay(30);

      Graph(DacVcc == 255, false, GetAdcSmooth(pin_ADC_PNP_Vcc), GetAdcSmooth(pin_ADC_PNP_Vce), base, Adc_12V);

      if (prev_y < 0)
        DacVcc = -1;
    };

    if (base > 0) {
      ILI9341SetCursor(prev_x + 2, prev_y + 2);
      switch (kind) {
        case tkPNP:
          DrawInt(base * 5, SmallFont, TFT_WHITE);
          DrawString("uA", SmallFont, TFT_WHITE);
          break;
        case tkPMOSFET:
          DrawDecimal(base, SmallFont, TFT_WHITE);
          DrawString("V", SmallFont, TFT_WHITE);
          break;
        case tkNJFET:
          break;
      }
    }
    SerialPrintLn("z");
    if (HaveTouch())
      return;
  }
  EndScan(kind);
}

//-------------------------------------------------------------------------
// ScanAllPos
//   draw curves for a component on the NPN side of the DUT socket
//     due to non-linearities, iFirst is different
//     Dac Base = iFirst
//            iConst+(1*iInc) / 10
//            iConst+(2*iInc) / 10
//            iConst+(3*iInc) / 10
//   if user touches the screen at the end of a line, the scan is terminated and the function returns true
//-------------------------------------------------------------------------

void ScanAllPos(TkindDUT kind, int iFirst, int iConst, int iInc, int minBase, int maxBase, int incBase) {

  int DacVcc; // collector
  int i, base;

  DacVcc = 0;
  maxYposGain = -1;
  minBaseGain = 0;
  maxBaseGain = 0;

  SerialPrintLn("n");

  for (base = minBase; base <= maxBase; base += incBase) {
    if (base == 0)
    {
      SetDacBase(iFirst, 0);
    } else {
      i = iConst + base * iInc / 10;
      if (i > 255) break;
      SetDacBase(i, 0);
    }

    for (DacVcc = 0; DacVcc <= 255; DacVcc += 2) {
      SetDacVcc(DacVcc, 1);
      if (DacVcc == 0)
        delay(30);

      Graph(DacVcc == 0, true, GetAdcSmooth(pin_ADC_NPN_Vcc), GetAdcSmooth(pin_ADC_NPN_Vce), base, ADC_MAX - 1);

      if (prev_y < 0)
        DacVcc = 256;
    };

    if (base > 0) {
      ILI9341SetCursor(prev_x + 1, prev_y + 2);
      switch (kind) {
        case tkNPN:
          DrawInt(base * 5, SmallFont, TFT_WHITE);
          DrawString("uA", SmallFont, TFT_WHITE);
          break;
        case tkNMOSFET:
          DrawDecimal(base, SmallFont, TFT_WHITE);
          DrawString("V", SmallFont, TFT_WHITE);
          break;
        case tkPJFET:
          break;
      }
    }
    SerialPrintLn("z");
    if (HaveTouch())
      return;
  }
  EndScan(kind);
}

//-------------------------------------------------------------------------
// ScanJFET
//   draw curves for a JFET
//   if user touches the screen at the end of a line, the scan is terminated and the function returns true
//-------------------------------------------------------------------------

void ScanJFET(TkindDUT kind, int minVgs, int maxVgs, int incVgs) {
  int DacVcc, Vgs;
  bool penDown;
  int DAC0, DACinc;
  int Adc_12V;

  DAC0 = (kind == tkNJFET) ? 255 : 0;
  DACinc = (kind == tkNJFET) ? -2 : +2;

  Adc_12V = GetAdcSmooth(pin_Adc_12V);

  for (Vgs = minVgs; Vgs <= maxVgs; Vgs += incVgs) {
    penDown = false;
    SetDacVcc(DAC0, 30);
    for (DacVcc = DAC0; DacVcc >= 0 && DacVcc <= 255; DacVcc += DACinc) {
      if (SetJFETvolts(kind, DacVcc, Vgs)) {
        if (kind == tkNJFET)
          Graph(!penDown, false, GetAdcSmooth(pin_ADC_PNP_Vcc), GetAdcSmooth(pin_ADC_PNP_Vce), Vgs, Adc_12V);
        else
          Graph(!penDown, true, GetAdcSmooth(pin_ADC_NPN_Vcc), GetAdcSmooth(pin_ADC_NPN_Vce), Vgs, ADC_MAX - 1);
        penDown = true;
      }
      else
        penDown = false;
    }

    ILI9341SetCursor(prev_x + 2, prev_y + 2);
    if (kind == tkNJFET)
      DrawDecimal(-Vgs, SmallFont, TFT_WHITE);
    else
      DrawDecimal(+Vgs, SmallFont, TFT_WHITE);
    DrawString("V", SmallFont, TFT_WHITE);
    SerialPrintLn("z");
    if (HaveTouch())
      return;
  }
  EndScan(kind);
}

//-------------------------------------------------------------------------
// ScanKind
//   draw curves for a component
//-------------------------------------------------------------------------

void ScanKind(TkindDUT kind) {
  int minBase, maxBase, incBase, Adc_12V, base_adj;

  Adc_12V = GetAdcSmooth(pin_Adc_12V);
  base_adj = Adc_12V / 4 - 255;
  base_adj = 0;

  InitGraph(kind);
  switch (kind) {
    case tkPNP:
    case tkNPN:
      minBase = MinIbase * 10 / 50;
      maxBase = MaxIbase * 10 / 50;
      break;

    case tkPMOSFET:
    case tkNMOSFET:
      minBase = MinVgate * 10;
      maxBase = MaxVgate * 10;
      break;

    case tkNJFET:
    case tkPJFET:
      minBase = MinVgate * 10;
      maxBase = MaxVgate * 10;
      break;
  }

  if (maxBase - minBase > 60) incBase = 10; else if (maxBase - minBase > 30) incBase = 5;  else if (maxBase - minBase > 20) incBase = 2;  else
    incBase = 1;

  switch (kind) {
    case tkPNP:     ScanAllNeg(kind, 255, 215 + base_adj, -27, minBase, maxBase, incBase, Adc_12V); break;
    case tkNPN:     ScanAllPos(kind, 0,   14,            27, minBase, maxBase, incBase);          break;

    case tkPMOSFET: ScanAllNeg(kind, 220, 220 + base_adj, -21, minBase, maxBase, incBase, Adc_12V); break;
    case tkNMOSFET: ScanAllPos(kind, 0,   0,             21, minBase, maxBase, incBase);          break;

    case tkNJFET:   ScanJFET(kind,                           minBase, maxBase, incBase);          break;
    case tkPJFET:   ScanJFET(kind,                           minBase, maxBase, incBase);          break;

    case tkPDiode:  ScanAllNeg(kind, 255, 255,           0,  0,       0,       10,      Adc_12V); break;
    case tkNDiode:  ScanAllPos(kind, 0,   0,             0,  0,       0,       10);               break;
  }
}

//-------------------------------------------------------------------------
// SquareWave
//    debugging - generates a square wave with both of the DACs
//-------------------------------------------------------------------------

void SquareWave(void) {
  while (true) {
    SetDacVcc(0, 0);
    SetDacBase(0, 50);
    SetDacVcc(0xFF, 0);
    SetDacBase(0xFF, 50);
  }
}

//-------------------------------------------------------------------------
// readSupply
//   calculate supply voltage of Arduino
//   uses internal bandgap reference
//-------------------------------------------------------------------------

long xreadSupply() {
  long result;
  int i;
  const int n = 10;
  result = 0;
  for (i = 1; i <= n; i++) {
    // Read 1.1V reference against AVcc
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    delay(2); // Wait for Vref to settle
    ADCSRA |= _BV(ADSC); // Convert
    while (bit_is_set(ADCSRA, ADSC));
    result += ADCL;
    result += ADCH << 8;
  }

  result = (n * 1125300L) / result; // Back-calculate AVcc in mV
  return result;
}

long readSupply() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;
}

//-------------------------------------------------------------------------
// BattVolts
//   Battery voltage  x 10
//-------------------------------------------------------------------------

int BattVolts() {
  return long(GetAdcSmooth(pin_Adc_Bat)) * readSupply() * (R6 + R7) / (100L * R7 * 1024);
}

//-------------------------------------------------------------------------
// PrintADCs
//   transmit ADC values on serial
//-------------------------------------------------------------------------

void  PrintADCs(void) {
  if (SendAdcValues) {
    SerialPrint("x ");
    SerialPrint(GetAdcSmooth(pin_ADC_PNP_Vcc));
    SerialPrint(",");
    SerialPrint(GetAdcSmooth(pin_ADC_PNP_Vce));
    SerialPrint(",");
    SerialPrint(GetAdcSmooth(pin_ADC_NPN_Vcc));
    SerialPrint(",");
    SerialPrint(GetAdcSmooth(pin_ADC_NPN_Vce));
    SerialPrint(",");
    SerialPrint(GetAdcSmooth(pin_Adc_12V));
    SerialPrint(",");
    SerialPrintLn(readSupply());
  }
}

//-------------------------------------------------------------------------
// ExecSerialCmd
//   execute a serial Rx command
//-------------------------------------------------------------------------

void ExecSerialCmd(void) {
  char c;
  if (Serial.available() == 0)
    return;

  switch (GetSerial()) {
    case 'A':
      SetDacVcc(GetSerial(), 2);
      break;

    case 'B':
      ExecSerialTx = true;
      SetDacBase(GetSerial(), 2);
      break;

    case 'N':
      ExecSerialTx = true;
      ScanKind(tkNPN);
      break;

    case 'P':
      ExecSerialTx = true;
      ScanKind(tkPNP);
      break;

    case 'F':
      ExecSerialTx = true;
      ScanKind(tkNMOSFET);
      break;

    case 'f':
      ExecSerialTx = true;
      ScanKind(tkPMOSFET);
      break;

    case 'J':
      ExecSerialTx = true;
      ScanKind(tkNJFET);
      break;

    case 'j':
      ExecSerialTx = true;
      ScanKind(tkPJFET);
      break;

    case 'Q':
      SquareWave();
      break;

    case 'D':
      ExecSerialTx = true;
      ScanKind(tkNDiode);
      break;

    case 'd':
      ExecSerialTx = true;
      ScanKind(tkPDiode);
      break;

    case 'K':
      int i, j;
      i = GetSerial(); i &= 0xFF;
      j = GetSerial(); j &= 0xFF;
      SetJFETvolts(tkNJFET, i, j);
      while (Serial.available() == 0) ;
      return;
      break;

    case 'H':
      Serial.println(TestDeviceKind(tkNothing, true));
      break;

    case 'X':
      SendAdcValues = true;
      break;

    case 'M':
      curkind = tkNothing;
      DrawMenuScreen();
      break;

    default:
      return;
  }

  ExecSerialTx = true;
}

//-------------------------------------------------------------------------
// MainMenuTouch
//   execute a touch command of main menu
//-------------------------------------------------------------------------

void MainMenuTouch(void) {
  int x, y;

  if (!GetTouch(&x, &y))
    return;

  if (y > TFT_HGT / 2) {
    if (x < TFT_WID / 3) {
      if (CurDUTclass != tcBipolar) {
        CurDUTclass = tcBipolar;
        DrawMenuScreen();
      }
    }
    else if (x < TFT_WID * 2 / 3) {
      if (CurDUTclass != tcMOSFET) {
        CurDUTclass = tcMOSFET;
        DrawMenuScreen();
      }
    } else {
      if (CurDUTclass != tcJFET) {
        CurDUTclass = tcJFET;
        DrawMenuScreen();
      }
    }
  }

  if (y < TFT_HGT / 4) {
    if (x > TFT_WID * 2 / 3) {
      switch (CurDUTclass) {
        case tcJFET:
        case tcMOSFET:
          if (ExecSetupMenuFET())
            return;
          break;

        default: //tcBipolar
          if (ExecSetupMenuBipolar())
            return;
      }
      DrawMenuScreen();
    }
  }
}

//-------------------------------------------------------------------------
// DrawCharColumn
//   draw a column of chars on main menu
//-------------------------------------------------------------------------

void DrawCharColumn(uint16_t x0, uint16_t y0, char* str, uint16_t color) {
  int y;
  char* c;

  y = y0;
  c = str;
  while (*c) {
    ILI9341SetCursor(x0, y);
    DrawChar(*c, SmallFont, color);
    y += 10;
    c++;
  }
}

//-------------------------------------------------------------------------
// DrawZIF
//   draw the socket
//-------------------------------------------------------------------------

void DrawZIF(char* str) {
  const int Shape1Width = 41;
  const int Shape1Height = 81;
  const int Shape1Left = (TFT_WID - Shape1Width) / 2;
  const int Shape1Top = 32;
  const int ShapeColor = RGB(0x80, 0xFF, 0x80);
  int x;

  DrawRoundRect(Shape1Left, Shape1Top + 5, Shape1Width, Shape1Height, 6, ShapeColor);

  for (x = Shape1Left - 8; x < 200; x += Shape1Width + 13)
    DrawCharColumn(x, Shape1Top + 17, str, TFT_WHITE);

  for (x = Shape1Left + 4; x < 200; x += Shape1Width - 13)
    DrawCharColumn(x, Shape1Top + 17, "ooooooo", TFT_BLACK);
}

//-------------------------------------------------------------------------
// DrawMenuScreen
//   draw the main menu screen
//-------------------------------------------------------------------------

void DrawMenuScreen(void) {
  ClearDisplay(TFT_BLACK);
  DrawKindStr(tkNothing);

  const int BipolarLeft = 10;
  const int MOSFETLeft = 114;
  const int JFETLeft = 218;

  DrawCheckBox(BipolarLeft, "PNP NPN", CurDUTclass == tcBipolar, RGB(0xFF, 0xFF, 0x80), bmpPNP, bmpNPN);
  DrawCheckBox(MOSFETLeft,  "MOSFET",  CurDUTclass == tcMOSFET,  RGB(0x80, 0xFF, 0xFF), bmpPMOSFET, bmpNMOSFET);
  DrawCheckBox(JFETLeft,    "JFET",    CurDUTclass == tcJFET,    RGB(0xFF, 0x80, 0xFF), bmpNJFET, bmpPJFET);

  char Str[] = "arduino";
  switch (CurDUTclass) {
    case tcMOSFET:
      DrawZIF("sgdsg");
      DrawStringAt(27,  94, "p-MOSFET", LargeFont, TFT_WHITE);
      DrawStringAt(200, 94, "n-MOSFET", LargeFont, TFT_WHITE);
      DrawBitmapMono(111, 43, bmpPDiodeSmall, TFT_WHITE);
      DrawBitmapMono(195, 43, bmpNDiodeSmall, TFT_WHITE);
      DrawBitmapMono(80, 43, bmpPMOSFET, TFT_WHITE);
      DrawBitmapMono(219, 43, bmpNMOSFET, TFT_WHITE);
      break;

    case tcJFET:
      DrawZIF("dgsdg");
      DrawStringAt(59,  94, "n-JFET", LargeFont, TFT_WHITE);
      DrawStringAt(200, 94, "p-JFET", LargeFont, TFT_WHITE);
      DrawBitmapMono(80, 43, bmpNJFET, TFT_WHITE);
      DrawBitmapMono(219, 43, bmpPJFET, TFT_WHITE);
      break;

    default: //tcBipolar
      DrawZIF("ebceb");
      DrawStringAt(83,  94, "PNP", LargeFont, TFT_WHITE);
      DrawStringAt(200, 94, "NPN", LargeFont, TFT_WHITE);
      DrawBitmapMono(111, 43, bmpPDiodeSmall, TFT_WHITE);
      DrawBitmapMono(195, 43, bmpNDiodeSmall, TFT_WHITE);
      DrawBitmapMono(80, 43, bmpPNP, TFT_WHITE);
      DrawBitmapMono(219, 43, bmpNPN, TFT_WHITE);
  }

  // draw buttons
  const int SetupWidth = 67;
  const int SetupHeight = 26;
  const int SetupLeft = TFT_WID - SetupWidth - 4;
  const int SetupTop = 4;

  DrawFrame(SetupLeft, SetupTop, SetupWidth, SetupHeight, TFT_WHITE);
  DrawBox(SetupLeft + 1, SetupTop + 1, SetupWidth - 2, SetupHeight - 2, TFT_DARKGREY);
  DrawStringAt(SetupLeft + 4, SetupTop + 19, "SETUP", LargeFont, TFT_BLACK);

  DrawStringAt(4,  15, "Bat ", LargeFont, RGB(128, 128, 255));
  DrawDecimal(BattVolts(), LargeFont, RGB(128, 128, 255));
}

//-------------------------------------------------------------------------
// DrawCheckBox
//   draw a CheckBox on the main menu screen
//-------------------------------------------------------------------------

void DrawCheckBox(int Left, char *str, bool checked, uint16_t color, const uint8_t *bitmap1, const uint8_t *bitmap2) {
  const int Top = 135;
  const int Width = 92;
  const int Height = 97;
  const int BoxWidth = 33;
  const int BoxHeight = 33;
  const int BoxLeft = (Width - BoxWidth) / 2;
  const int BoxTop = 52;

  pen_width = 2;
  DrawFrame(Left, Top, Width, Height, TFT_WHITE);
  pen_width = 1;
  DrawBox(Left + 2, Top + 2, Width - 4, Height - 4, color);

  pen_width = 2;
  DrawFrame(Left + BoxLeft, Top + BoxTop, BoxWidth, BoxHeight, TFT_BLACK);
  pen_width = 1;
  DrawBox(Left + BoxLeft + 2, Top + BoxTop + 2, BoxWidth - 4, BoxHeight - 4, TFT_WHITE);
  if (checked) {
    pen_width = 2;
    DrawLine(Left + BoxLeft, Top + BoxTop, Left + BoxLeft + BoxWidth - 1, Top + BoxTop + BoxHeight - 1, TFT_BLACK);
    DrawLine(Left + BoxLeft, Top + BoxTop + BoxHeight - 1, Left + BoxLeft + BoxWidth - 1, Top + BoxTop, TFT_BLACK);
    pen_width = 1;
  }

  execDrawChar = false;
  DrawStringAt(0, Top + 15, str, LargeFont, TFT_BLACK);
  execDrawChar = true;
  DrawStringAt(Left + (Width - Cursorx) / 2, Top + 15, str, LargeFont, TFT_BLACK);

  if (bitmap1)
    DrawBitmapMono(Left + 8, Top + 18, bitmap1, TFT_BLACK);

  if (bitmap2)
    DrawBitmapMono(Left + Width - 28 - 9, Top + 18, bitmap2, TFT_BLACK);
}

//-------------------------------------------------------------------------
// ExecSetupMenu
//   draw and executes the setup menu screen
//   returns true if a DUT is inserted
//-------------------------------------------------------------------------

bool ExecSetupMenu(char *str1, char *str2, char *str3, int *amin, int *amax, int valMax, int valInc) {
  int x, y;
  static unsigned long time = 0;

  while (true) {
    ClearDisplay(TFT_BLACK);
    DrawStringAt((TFT_WID - 117) / 2, 15, str1, LargeFont, TFT_YELLOW);

    const int TextLeft = 40;
    const int TextTop = 56 + 10 + 4;
    const int ValLeft = 148;
    const int BoxTop = 48;
    const int BoxLeft = 176;
    const int RowHeight = 50;
    DrawStringAt(TextLeft, TextTop, str2, LargeFont, TFT_WHITE);
    ILI9341SetCursor(ValLeft, TextTop); DrawInt(*amin, LargeFont, TFT_WHITE);
    DrawFrame(BoxLeft, BoxTop, 34, 34, TFT_WHITE);
    DrawBitmapMono(BoxLeft + 5, BoxTop + 9, bmpUpDownArrow, TFT_WHITE);

    DrawStringAt(TextLeft, TextTop + RowHeight, str3, LargeFont, TFT_WHITE);
    ILI9341SetCursor(ValLeft, TextTop + RowHeight); DrawInt(*amax, LargeFont, TFT_WHITE);
    DrawFrame(BoxLeft, BoxTop + RowHeight, 34, 34, TFT_WHITE);
    DrawBitmapMono(BoxLeft + 5, BoxTop + 9 + RowHeight, bmpUpDownArrow, TFT_WHITE);

    const int OKWidth = 61;
    const int OKHeight = 34;
    const int OKLeft = (TFT_WID - OKWidth) / 2;
    const int OKTop = TFT_HGT - OKHeight - 8;
    DrawFrame(OKLeft, OKTop, OKWidth, OKHeight, TFT_WHITE);
    DrawBox(OKLeft + 2, OKTop + 2, OKWidth - 4, OKHeight - 4, TFT_WHITE);
    DrawStringAt(OKLeft + 17, OKTop + 22, "OK", LargeFont, TFT_BLACK);

    while (true) {
      if (GetTouch(&x, &y)) {
        if (y > TFT_HGT - 80)
          return false;

        if (y > BoxTop + RowHeight) {
          if (*amax < valMax)
            *amax += valInc;
          else
            *amax = *amin + valInc;
          break;
        }

        if (y > BoxTop) {
          if (*amin < *amax - valInc)
            *amin += valInc;
          else
            *amin = 0;
          break;
        }
      }

      if (millis() - time > 1000) {
        if (TestDeviceKind(tkNothing, false) != tkNothing)
          return true;
        time = millis();
      }
    }
  }
}

//-------------------------------------------------------------------------
// ExecSetupMenuFET
//   draw and executes the setup menu screen for a FET
//-------------------------------------------------------------------------

bool ExecSetupMenuFET(void) {
  return ExecSetupMenu("  FET Setup", "Min V-gate", "Max V-gate", &MinVgate, &MaxVgate, 12, 1);
}

//-------------------------------------------------------------------------
// ExecSetupMenuBipolar
//   draw and executes the setup menu screen for a Bipolar
//-------------------------------------------------------------------------

bool ExecSetupMenuBipolar(void) {
  return ExecSetupMenu("Bipolar Setup", "Min I-base", "Max I-base", &MinIbase, &MaxIbase, 350, 50);
}

//-------------------------------------------------------------------------
// setup
//   Arduino setup function
//-------------------------------------------------------------------------

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pin_DAC_CS, OUTPUT);

  ILI9341fast = false;
  ILI9341Begin(2, 4, 3, TFT_WID, TFT_HGT, ILI9341_Rotation3);
  BeginTouch(7, 3);

  Serial.begin(9600);
  Serial.println("Curve Tracer");

  SPI.begin();
  SPI.beginTransaction(SPISettings (4000000UL, MSBFIRST, SPI_MODE0));  // SPI for DAC

  TurnOffLoad(tkNothing);
  DrawMenuScreen();
}

//-------------------------------------------------------------------------
// loop
//   Arduino loop function
//-------------------------------------------------------------------------

void loop(void) {
  static unsigned long time = 0;
  TkindDUT kind;

  ExecSerialCmd();

  PrintADCs();

  if (millis() - time > 1000 && !ExecSerialTx) {
    kind = TestDeviceKind(curkind, false);
    if ((curkind == tkNothing) && kind != tkNothing) {
      delay(500); // so component is fully inserted
      //Serial.println('a');
      kind = TestDeviceKind(kind, true);
      if (kind != tkNothing) {
        ScanKind(kind);
      }
      curkind = kind;
    }
    else if ((curkind != tkNothing) && kind == tkNothing) {
      DrawMenuScreen();
      curkind = tkNothing;
    }

    if (HaveTouch() && (curkind != tkNothing)) {
      DrawMenuScreen();
      curkind = tkNothing;
    }
    time = millis();
  }

  if (curkind == tkNothing) {
    MainMenuTouch();
    if (!ExecSerialTx)
      TurnOffLoad(tkNothing);
  }
}

