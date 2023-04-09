
#ifndef _HELP_H_
#define _HELP_H_

#define CMDERR 0
#define NUMERR 1
#define ERROR_2 2
#define ERROR_3 3
#define ERROR_4 4
#define ERROR_5 5
#define ERROR_6 6
#define ERROR_7 7

void printError(unsigned int errno);
void printHelp(void);
void printBanner(void);
void pgmMessage(const char *msg);

#endif  // _HELP_H_