#ifndef __SIO_H__
#define __SIO_H__

int sioPutchar(int ch);
int sioGetchar(void);
int sioWaitGetchar(void);
void sioSetTimeout(int timeout);
void sioSetBaud(int baud);
void sioInit(void);
void sioClose(void);

void sioEnableIntr();
void sioDisableIntr();

void sioErrorPrint();
#endif
