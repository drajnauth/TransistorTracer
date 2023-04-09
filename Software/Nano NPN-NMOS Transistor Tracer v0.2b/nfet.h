#ifndef _NFET_H_
#define _NFET_H_

void doInputSweep(void);
void doOutputFETSweep(void);
void doFETKSweep(void);
void getVgsOn(float min, float max, float inc);

#endif  // _NFET_H_