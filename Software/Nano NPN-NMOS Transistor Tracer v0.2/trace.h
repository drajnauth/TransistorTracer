#ifndef _TRACE_H_
#define _TRACE_H_

void resetPins(void);
void resetVoltages(void);
void setDefaultLimits(void);
void clearSweepFlags(void);

int adcAverage(int pin, unsigned char average);
void setUpPWM(void);
void setVoltage(int pin, float voltage);
void displayVoltages(void);
void measureVoltages(void);
void getADCValues(void);
void setVbCurrent(float current);
void setVcVoltage(float voltage);
void getMaxVoltages(void);
void printSweepValues(char type);
void printConfig(char type);
unsigned char checkSweepRange(void);
unsigned char checkResistors(char type, char announce);
unsigned char checkVcc(char type);
float getVccVoltage(void);
void printResistorValues(char type);

#endif  // _TRACE_H_