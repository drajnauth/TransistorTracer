#ifndef _TRACE_H_
#define _TRACE_H_

void resetPins(void);
void resetVoltages(void);
void setDefaultLimits(void);
void clearSweepFlags(void);

void displayVoltages(void);

void getMinMaxVoltages(void);
void printControlValues(char type);
void printConfig(char type);
void printOutputVoltageValues(char type);
unsigned char checkSweepRange(void);
unsigned char checkResistors(char type, char announce);

int adcAverage(int pin, unsigned char average);
void setUpPWM(void);
void setVoltage(int pin, float voltage);
void measureVoltages(void);
float convertControlToOutput(float vol, char type);
float convertOutputToControl(float vol, char type);
float getVccVoltage(void);
void waitForVcc(char type);
float checkVcc(char type);
void printResistorValues(char type);

int calculatePWM(float voltage);

#endif  // _TRACE_H_