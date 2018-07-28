#include <pspkernel.h>
#include <psphprm.h>
#include <pspsyscon.h>

/* MINI UART DRIVER */
#define PSP_UART4_FIFO 0xBE500000
#define PSP_UART4_STAT 0xBE500018
#define PSP_UART4_DIV1 0xBE500024
#define PSP_UART4_DIV2 0xBE500028
#define PSP_UART4_CTRL 0xBE50002C
#define PSP_UART_CLK   96000000
#define PSP_UART_TXFULL  0x20
#define PSP_UART_RXEMPTY 0x10

int sceHprmEnd(void);
int sceHprmInit(void);
int sceSysregUartIoEnable(int uart);
int sceSysregUartIoDisable(int uart);

#define SIOMAXERRORS 10
char sioErrorMessage[SIOMAXERRORS][128] = { {0}, {0}, {0}, {0} };
int sioErrorIndex = 0;

int sioTimeout = 100000;

int sioPutchar(int ch)
{
	int i = 0;
	while((sioTimeout > 0 && i < sioTimeout) && (_lw(PSP_UART4_STAT) & PSP_UART_TXFULL)) i++;

	//sprintf(sioErrorMessage[sioErrorIndex],"sioPutchar: timeout value = %i\n", i);
	//sioErrorIndex = (sioErrorIndex+1)%SIOMAXERRORS;
	if (sioTimeout > 0 && i >= sioTimeout)
	{
		sprintf(sioErrorMessage[sioErrorIndex],"sioPutchar: SIO timeout!\n");
		sioErrorIndex = (sioErrorIndex+1)%SIOMAXERRORS;
		return -1;
	}

	_sw(ch, PSP_UART4_FIFO);
	return i;
}

int sioGetchar(void)
{
	if(_lw(PSP_UART4_STAT) & PSP_UART_RXEMPTY)
	{
		return -1;
	}

	return _lw(PSP_UART4_FIFO);
}

int sioWaitGetchar()
{
	int i = 0;
	while((sioTimeout > 0 && i < sioTimeout) && (_lw(PSP_UART4_STAT) & PSP_UART_RXEMPTY)) i++; // loop until rx buffer full

	//sprintf(sioErrorMessage[sioErrorIndex],"sioWaitGetchar: timeout value = %i\n", i);
	//sioErrorIndex = (sioErrorIndex+1)%SIOMAXERRORS;
	if (sioTimeout > 0 && i >= sioTimeout)
	{
		sprintf(sioErrorMessage[sioErrorIndex],"sioWaitGetchar: SIO timeout!\n");
		sioErrorIndex = (sioErrorIndex+1)%SIOMAXERRORS;
		return -1;
	}

	return _lw(PSP_UART4_FIFO);
}

void sioErrorPrint()
{
	int i = sioErrorIndex;
	int j = 0;
	while (j++<SIOMAXERRORS)
	{
		i = (i+1)%SIOMAXERRORS;
		printf(sioErrorMessage[i]);
		sioErrorMessage[i][0] = 0;
	}
}

void sioSetTimeout(int timeout)
{
	sioTimeout = timeout;
}

void sioSetBaud(int baud)
{
	int div1, div2;

	/* rate set using the rough formula div1 = (PSP_UART_CLK / baud) >> 6 and
	 * div2 = (PSP_UART_CLK / baud) & 0x3F
	 * The uart4 driver actually uses a slightly different formula for div 2 (it
	 * adds 32 before doing the AND, but it doesn't seem to make a difference
	 */
	div1 = PSP_UART_CLK / baud;
	div2 = div1 & 0x3F;
	div1 >>= 6;

	_sw(div1, PSP_UART4_DIV1);
	_sw(div2, PSP_UART4_DIV2);
	_sw(0x60, PSP_UART4_CTRL);
}


int intr_handler(void *arg)
{
	// Read out the interrupt state and clear it
	u32 stat = _lw(0xBE500040);
	_sw(stat, 0xBE500044);
	
	sprintf(sioErrorMessage[sioErrorIndex],"INT stat: %i (0x%X)\n", stat, stat);
	sioErrorIndex = (sioErrorIndex+1)%SIOMAXERRORS;
	//sceKernelDisableIntr(PSP_HPREMOTE_INT);
	//sceKernelSetEventFlag(g_eventflag, 0x01);
	
	return -1;
}

SceUID g_eventflag = -1;

int sioEnableIntr()
{
	//g_eventflag = sceKernelCreateEventFlag("SioShellEvent", 0, 0, 0);
	void *func = (void *) ((unsigned int) intr_handler | 0x80000000);
	int st = sceKernelRegisterIntrHandler(PSP_HPREMOTE_INT, 1, func, NULL, NULL);
	printf("sceKernelRegisterIntrHandler = %i\n", st);
	st = sceKernelEnableIntr(PSP_HPREMOTE_INT);
	// Delay thread for a bit
	sceKernelDelayThread(2000000);
}

int sioDisableIntr()
{
	sceKernelDisableIntr(PSP_HPREMOTE_INT);
	sceKernelReleaseIntrHandler(PSP_HPREMOTE_INT);
	//sceKernelEnableIntr(PSP_HPREMOTE_INT);
	//sceKernelWaitEventFlag(g_eventflag, 0x01, 0x21, &result, &timeout);
}


static int sioInitialized = 0;
void sioInit(void)
{
	if (sioInitialized) return;
	/* Shut down the remote driver */
	sceHprmEnd();

	/* Enable UART 4 */
	sceSysregUartIoEnable(4);
	/* Enable remote control power */
	sceSysconCtrlHRPower(1);
	/* Delay thread for a but */
	//sceKernelDelayThread(2000000);
	sioInitialized = 1;
}

void sioClose(void)
{
	if (!sioInitialized) return;
	sceSysregUartIoDisable(4);
	sceHprmInit();
	//sceHprmResume();
	//sceSysconCtrlHRPower(-1);
	//sceKernelDelayThread(2000000);
	sioInitialized = 0;
}
