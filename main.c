#include <pspkernel.h>
#include <pspinit.h>
#include <pspsdk.h>
#include <psprtc.h>
#include <pspumd.h>
#include <psphprm.h>
#include <pspsyscon.h>
#include <psppower.h>
#include <pspdisplay.h>
#include <pspdisplay_kernel.h>
#include <pspctrl.h>
#include <pspctrl_kernel.h>
#include <psputilsforkernel.h>
#include <pspiofilemgr.h>
#include <pspimpose_driver.h>

#include <string.h>

#ifndef NOLIBM
#include <math.h>
#else
#include "minimath.h"
#endif

#include "hook.h"
#include "sio.h"
#include "debug.h"
#include "print.h"
#include "config.h"
#include "utils.h"

#include "sdk/motion.h"



PSP_MODULE_INFO("motion_driver", 0x1006, 1, 0);


int sceUmdDeactivate(int unit, const char *drive);

int (*g_ctrl_common)(SceCtrlData *, int count, int type);
int (*g_setframebuf)(int unk, void* addr, int width, int psm, int sync);


extern int sceCodecOutputEnable(int enable_headphone, int enable_speaker);

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define CLAMP127(x) ((x) < -128 ? - 128 : (x) > 127 ? 127 : (x))


#define AXIS_SHIFT	4
#define POS_X	0x00
#define NEG_X	0x01
#define POS_Y	0x10
#define NEG_Y	0x11
#define POS_Z	0x20
#define NEG_Z	0x21
#define ROT_OFF	0x30

typedef union
{
	struct {
		int		posxButtons, negxButtons;
		int		posyButtons, negyButtons;
		int		poszButtons, negzButtons;
		
		int		posxButtons2, negxButtons2;
		int		posyButtons2, negyButtons2;
		int		poszButtons2, negzButtons2;
	};
	int	Buttons[6][2];
} btnConfig;

const int		g_version = 0x0100000b;
static int		g_debug = 1;
static int		g_info = 0;
static int		g_quit = 0;
static int		g_savesettings = 0;
static char*	g_executable = 0;
static char	g_stringbuffer[128];
static int		g_motionenabled = 1;
static int		g_motionexists = 0;
static int		g_motioninputs = 0;
static int		g_motioninputavailable = 0;
static int		g_motionforward = 1;
static int		g_filterwidth = 1;
static float	g_filterweight = 0.5f;
static float	g_filterpow = 1.5f;
static int		g_samplingcycles = 50000;
static int		g_accelthreshold = 10;
static int		g_rotthreshold[3] = { 16, 8, 8 };
static int		g_rotoffset[3] = { 20, 0, 0 };
static btnConfig g_buttons = { { 0,0,0,0,0,0, 0,0,0,0,0,0 } };
static btnConfig g_timeouts = { { 0,0,0,0,0,0, 0,0,0,0,0,0 } };
static int		g_muted = 0;

#define ACCELBUFFER_SIZE 16
static motionAccelData	g_accelbuffer[ACCELBUFFER_SIZE];
static int				g_accelindex = 0;
static motionAccelData	g_currot = { .value = 0x80808080 };
static motionAccelData	g_curaccel = { .value = 0x80808080 };
//static motionAccelData	g_lastaccel = { .value = 0x80808080 };
static motionAccelData	g_nullaccel = { .value = 0x80808080 };

static char* directions[3] = { "X AXIS", "Y AXIS", "Z AXIS" };
static unsigned int last_frame = 0;

const int g_ButtonTimeout = 20;
//const int g_ButtonDelay = 4;

int buttonpressed = 0;
int resetoutput = 0;

void adjust_values(SceCtrlData *pad_data, int count, int neg)
{
	if (!(g_motionforward && g_motionexists && g_motionenabled && g_motioninputavailable)) return;

	int intc = pspSdkDisableInterrupts();
	int i;
	int accelpress = 0;
	for(i = 0; i < count; i++)
	{
		int buttons = (neg?~pad_data[i].Buttons:pad_data[i].Buttons);
		
		if (buttons&PSP_CTRL_HOLD)
		{
			pad_data[i].Lx = 128;
			pad_data[i].Ly = 128;
			continue;
		}
		if (buttons&(PSP_CTRL_NOTE|PSP_CTRL_VOLUP|PSP_CTRL_VOLDOWN))
		{
			if (g_motionexists)
				resetoutput = 5;
		}

		int j = 0;
		if (!buttonpressed && g_debug) DEBUG_PRINTF("ACCEL: (%i,%i,%i)\nNULL: (%i,%i,%i)\nROT: (%i,%i,%i)\n", g_curaccel.x-128, g_curaccel.y-128, g_curaccel.z-128, g_nullaccel.x-128, g_nullaccel.y-128, g_nullaccel.z-128, g_currot.x-128, g_currot.y-128, g_currot.z-128 );
		int accel = g_curaccel.value;
		for (j=0;j<3;j++)
		{
			if (g_buttons.Buttons[j][0]|g_buttons.Buttons[j][1])
			{
				if ((g_buttons.Buttons[j][0]|g_buttons.Buttons[j][1])&(PSP_CTRL_ANALOG_LEFT|PSP_CTRL_ANALOG_RIGHT))
					pad_data[i].Lx = accel & 0xFF;
				else
				if ((g_buttons.Buttons[j][0]|g_buttons.Buttons[j][1])&(PSP_CTRL_ANALOG_UP|PSP_CTRL_ANALOG_DOWN))
					pad_data[i].Ly = accel & 0xFF;

				if ((int)(accel&0xFF) > 128+g_accelthreshold)
				{
					if (g_timeouts.Buttons[j][0]==0)
					{
						scePowerTick(0);
						buttons |= (g_buttons.Buttons[j][0]&0x3FFFFFF);
						buttonpressed = 1;
						accelpress = 2;
						int k,l;
						for (k=3;k<6;k++)
						for (l=0;l<2;l++)
							g_timeouts.Buttons[k][l] = g_ButtonTimeout;
						if (g_debug) DEBUG_PRINTF("POSITIVE %s PUSH %i\n",directions[j],(accel&0xFF)-128);
						g_timeouts.Buttons[j][1] = g_ButtonTimeout;
					}
				}
				else
				if ((int)(accel&0xFF) < 128-g_accelthreshold)
				{
					if (g_timeouts.Buttons[j][1]==0)
					{
						scePowerTick(0);
						buttons |= (g_buttons.Buttons[j][1]&0x3FFFFFF);
						buttonpressed = 1;
						accelpress = 2;
						int k,l;
						for (k=3;k<6;k++)
						for (l=0;l<2;l++)
							g_timeouts.Buttons[k][l] = g_ButtonTimeout;
						if (g_debug) DEBUG_PRINTF("NEGATIVE %s PUSH %i\n",directions[j],(accel&0xFF)-128);
						g_timeouts.Buttons[j][0] = g_ButtonTimeout;
					}
				}
				
				if (pad_data[0].TimeStamp>last_frame)
				{
					if (g_timeouts.Buttons[j][0]>0)
						g_timeouts.Buttons[j][0]--;
					
					if (g_timeouts.Buttons[j][1]>0)
						g_timeouts.Buttons[j][1]--;
				}
			}
			accel >>= 8;
		}

		accel = g_currot.value;
		for (j=3;!accelpress && j<6;j++)
		{
			if (g_buttons.Buttons[j][0]|g_buttons.Buttons[j][1])
			{
				int push = (int)((accel + g_rotoffset[j-3])&0xFF)-128;
				if (push>g_rotthreshold[j-3])
				{
					if (g_timeouts.Buttons[j][0]==0)
					{
						scePowerTick(0);
						buttons |= (g_buttons.Buttons[j][0]&0x3FFFFFF);
						buttonpressed = 1;
						if (g_debug) DEBUG_PRINTF("POSITIVE %s ROTATION %i\n",directions[j-3],push);
						g_timeouts.Buttons[j][1] = g_ButtonTimeout;
					}
				}
				else
				if (push<-g_rotthreshold[j-3])
				{
					if (g_timeouts.Buttons[j][1]==0)
					{
						scePowerTick(0);
						buttons |= (g_buttons.Buttons[j][1]&0x3FFFFFF);
						buttonpressed = 1;
						if (g_debug) DEBUG_PRINTF("NEGATIVE %s ROTATION %i\n",directions[j-3],push);
						g_timeouts.Buttons[j][0] = g_ButtonTimeout;
					}
				}

				if (pad_data[0].TimeStamp>last_frame)
				{
					if (g_timeouts.Buttons[j][0]>0)
						g_timeouts.Buttons[j][0]--;

					if (g_timeouts.Buttons[j][1]>0)
						g_timeouts.Buttons[j][1]--;
				}
			}
			accel >>= 8;
		}
		
		pad_data[i].Buttons = (neg?~buttons:buttons);
	}
	if (pad_data[0].TimeStamp>last_frame && accelpress>0)
		accelpress--;
	last_frame = pad_data[0].TimeStamp;
	pspSdkEnableInterrupts(intc);
}



int ctrl_hook_func(SceCtrlData *pad_data, int count, int type)
{
	int ret = g_ctrl_common(pad_data, count, type);
	if(ret <= 0)
	{
		return ret;
	}

	adjust_values(pad_data, ret, type&1);

	return ret;
}


int setframebuf_hook_func(int unk, void* addr, int width, int psm, int sync)
{
	if (g_info)
	{
		scrprint( 2, 2, debugmsg, addr, psm );
		g_info--;
		if (!g_info)
		{
			DEBUG_RESET();
			buttonpressed = 0;
		}
	}
	
	return g_setframebuf(unk, addr, width, psm, sync);
}



int map_button( char* str )
{
	if (strncmpupr(str,"START",5)==0)
		return PSP_CTRL_START;
	if (strncmpupr(str,"SELECT",6)==0)
		return PSP_CTRL_SELECT;
	
	if (strncmpupr(str,"RTRIGGER",8)==0)
		return PSP_CTRL_RTRIGGER;
	if (strncmpupr(str,"LTRIGGER",8)==0)
		return PSP_CTRL_LTRIGGER;
	
	if (strncmpupr(str,"CROSS",5)==0)
		return PSP_CTRL_CROSS;
	if (strncmpupr(str,"SQUARE",6)==0)
		return PSP_CTRL_SQUARE;
	if (strncmpupr(str,"CIRCLE",6)==0)
		return PSP_CTRL_CIRCLE;
	if (strncmpupr(str,"TRIANGLE",8)==0)
		return PSP_CTRL_TRIANGLE;
	
	if (strncmpupr(str,"LEFT",4)==0)
		return PSP_CTRL_LEFT;
	if (strncmpupr(str,"RIGHT",5)==0)
		return PSP_CTRL_RIGHT;
	if (strncmpupr(str,"UP",2)==0)
		return PSP_CTRL_UP;
	if (strncmpupr(str,"DOWN",4)==0)
		return PSP_CTRL_DOWN;

	if (strncmpupr(str,"ANALOGLEFT",10)==0)
		return PSP_CTRL_ANALOG_LEFT;
	if (strncmpupr(str,"ANALOGRIGHT",11)==0)
		return PSP_CTRL_ANALOG_RIGHT;
	if (strncmpupr(str,"ANALOGUP",8)==0)
		return PSP_CTRL_ANALOG_UP;
	if (strncmpupr(str,"ANALOGDOWN",10)==0)
		return PSP_CTRL_ANALOG_DOWN;
		
	return 0;
}


void cfgcallback( char* element, char* value )
{
	printf("Read config> %s = %s\n", element, value );
	if (strncmpupr(element,"POS",3)==0 || strncmpupr(element,"NEG",3)==0)
	{
		int val = 0;
		value = strtok( value, "|\n" );
		while (value)
		{
			while ((*value==' ' || *value=='\t' || *value=='\n') && *value!=0)
				value++;
			printf("token \"%s\"\n", value);
			val |= map_button( value );
			value = strtok( NULL, "|\n" );
		}

		int index1 = 0;
		if (strncmpupr(element,"NEG",3)==0) index1 += 1;
		if (element[3]=='Y' || element[3]=='y') index1 += 0x10;
		else
		if (element[3]=='Z' || element[3]=='z') index1 += 0x20;
		else
		if (element[3]!='X' && element[3]!='x') return;
		if (strncmpupr(element+4,"ROT",3)==0) index1 += ROT_OFF;
		
		printf("Set button index 0x%x to 0x%08X\n", index1, val);
		motionSetForward( index1, val ); //g_buttons.Buttons[index0][index1] = val;
	}
	else
	if (strncmpupr(element,"ENABLED",7)==0)
	{
		g_motionenabled = atoi(value)?1:0;
	}
	else
	if (strncmpupr(element,"FILTER",6)==0)
	{
		if (strncmpupr(element+6,"WIDTH",5)==0)
		{
			motionSetFilter( atoi(value), g_filterweight );
		}
		else
		if (strncmpupr(element+6,"WEIGHT",6)==0)
		{
			motionSetFilter( g_filterwidth, atof(value) );
		}
		else
		if (strncmpupr(element+6,"POW",3)==0)
		{
			motionSetPow( atof(value) );
		}
	}
	else
	if (strncmpupr(element,"SAMPLING",8)==0)
	{
		motionSetSampling( atoi(value) );
	}
	else
	if (strncmpupr(element,"ACCELTHRESHOLD",14)==0)
	{
		g_accelthreshold = atoi(value);
		if (g_accelthreshold<4) g_accelthreshold = 4;
		if (g_accelthreshold>124) g_accelthreshold = 124;
	}
	else
	if (strncmpupr(element,"ROT",3)==0)
	{
		int index = (element[3]>='X'&&element[3]<='Z')?element[3]-'X':element[3]-'x';
		
		if (strncmpupr(element+4,"THRESHOLD",9)==0)
		{
			g_rotthreshold[index] = atoi(value);
			if (g_rotthreshold[index]<4) g_rotthreshold[index] = 4;
			if (g_rotthreshold[index]>124) g_rotthreshold[index] = 124;
		}
		else
		if (strncmpupr(element+4,"OFFSET",6)==0)
		{
			g_rotoffset[index] = atoi(value);
			if (g_rotoffset[index]<-64) g_rotoffset[index] = -64;
			if (g_rotoffset[index]>64) g_rotoffset[index] = 64;
		}
	}
	else
	if (strncmpupr(element,"DEBUGINFO",9)==0)
	{
		g_debug = atoi(value)?1:0;
	}
}

int load_config()
{
	/*
	//g_buttons.negxButtons = PSP_CTRL_RIGHT;
	//g_buttons.posxButtons = PSP_CTRL_LEFT;
	//g_buttons.negyButtons = PSP_CTRL_UP;
	//g_buttons.posyButtons = PSP_CTRL_DOWN;
	g_buttons.negzButtons = PSP_CTRL_CROSS;
	g_buttons.poszButtons = PSP_CTRL_CIRCLE;


	g_buttons.negxButtons2 = PSP_CTRL_UP;
	g_buttons.posxButtons2 = PSP_CTRL_DOWN;
	//g_buttons.negyButtons2 = PSP_CTRL_LTRIGGER;
	//g_buttons.posyButtons2 = PSP_CTRL_RTRIGGER;
	g_buttons.negzButtons2 = PSP_CTRL_RIGHT;
	g_buttons.poszButtons2 = PSP_CTRL_LEFT;
	//g_buttons.negzButtons2 = PSP_CTRL_CROSS;
	//g_buttons.poszButtons2 = PSP_CTRL_CIRCLE;
	*/
	printf("LOADING DEFAULT CONFIG>\n");
	cfgReadCallback( "ms0:/SEPLUGINS/motion_driver.ini", "DEFAULT", cfgcallback );
	printf("LOADING %s CONFIG>\n",g_executable);
	cfgReadCallback( "ms0:/SEPLUGINS/motion_driver.ini", g_executable, cfgcallback );
	
	printf("SETTINGS:\n");
	printf("posxButtons: 0x%08X\n", g_buttons.posxButtons);
	printf("negxButtons: 0x%08X\n", g_buttons.negxButtons);
	printf("posyButtons: 0x%08X\n", g_buttons.posyButtons);
	printf("negyButtons: 0x%08X\n", g_buttons.negyButtons);
	printf("poszButtons: 0x%08X\n", g_buttons.poszButtons);
	printf("negzButtons: 0x%08X\n", g_buttons.negzButtons);
	printf("posxButtons2: 0x%08X\n", g_buttons.posxButtons2);
	printf("negxButtons2: 0x%08X\n", g_buttons.negxButtons2);
	printf("posyButtons2: 0x%08X\n", g_buttons.posyButtons2);
	printf("negyButtons2: 0x%08X\n", g_buttons.negyButtons2);
	printf("poszButtons2: 0x%08X\n", g_buttons.poszButtons2);
	printf("negzButtons2: 0x%08X\n", g_buttons.negzButtons2);
	printf("rotoffsets: %i, %i, %i\n", g_rotoffset[0], g_rotoffset[1], g_rotoffset[2]);
	printf("rotthreshold: %i, %i, %i\n", g_rotthreshold[0], g_rotthreshold[1], g_rotthreshold[2]);
	printf("accelthreshold: %i\n", g_accelthreshold);
	return 1;
}

char* button( int btn )
{
	switch(btn)
	{
		case PSP_CTRL_START:	return "START";
		case PSP_CTRL_SELECT:	return "SELECT";
		case PSP_CTRL_LTRIGGER:	return "LTRIGGER";
		case PSP_CTRL_RTRIGGER:	return "RTRIGGER";
		case PSP_CTRL_LEFT:		return "LEFT";
		case PSP_CTRL_RIGHT:	return "RIGHT";
		case PSP_CTRL_UP:		return "UP";
		case PSP_CTRL_DOWN:		return "DOWN";
		case PSP_CTRL_TRIANGLE:	return "TRIANGLE";
		case PSP_CTRL_CROSS:	return "CROSS";
		case PSP_CTRL_SQUARE:	return "SQUARE";
		case PSP_CTRL_CIRCLE:	return "CIRCLE";
		case PSP_CTRL_SCREEN:	return "SCREEN";
		case PSP_CTRL_VOLUP:	return "VOLUP";
		case PSP_CTRL_VOLDOWN:	return "VOLDOWN";
		case PSP_CTRL_NOTE:		return "NOTE";
		case PSP_CTRL_ANALOG_LEFT:	return "ANALOGLEFT";
		case PSP_CTRL_ANALOG_RIGHT:	return "ANALOGRIGHT";
		case PSP_CTRL_ANALOG_UP:	return "ANALOGUP";
		case PSP_CTRL_ANALOG_DOWN:	return "ANALOGDOWN";
		default:				return "";
	}
}

int save_config()
{
	return 1;
}


void initaccelbuffer()
{
	int i;
	for (i=0;i<ACCELBUFFER_SIZE;i++)
	{
		g_accelbuffer[i].value = 0x80808080;
	}
	g_accelindex = 0;
}


void hprmResetUART( int baud )
{
	int div1 = 96000000 / baud;
	int div2 = div1 & 0x3F;
	div1 >>= 6;
	
	_sw(768,0xBE500030);
	_sw(div1,0xBE500024);	// Set baud 4800
	_sw(div2,0xBE500028);	// "
	_sw(112,0xBE50002C);	// "
	_sw(  0,0xBE500034);
	_sw( 16,0xBE500038);
	int v1 = _lw(0xBE500004);
	_sw(v1,0xBE500004);
	_sw(769,0xBE500030);
}


static int g_hprmRemote = -1;
static int g_hprmHeadphone = -1;


extern int sceSysregUartIoEnable(int uart);
extern int	sceSysconGetHPConnect(void);
extern int sceSyscon_driver_BBFB70C0( void (*)(int), int unk0 );
extern int sceSyscon_driver_805180D1( void (*)(int), int unk0 );
#define sceSysconSetHPConnectCallback sceSyscon_driver_BBFB70C0
#define sceSysconSetHRPowerCallback sceSyscon_driver_805180D1
extern int sceHprmInit();
extern int sceHprmEnd();

void HRPowerCallback( int stat )
{
	g_hprmRemote = stat;
}

void HPConnectCallback( int stat )
{
	if (stat)
	{
		sceSysconCtrlHRPower(1);
		//printf("Headphone plugged in.\n");
		g_hprmRemote = -1;
		g_hprmHeadphone = 1;
	}
	else
	{
		//printf("Headphone plugged out.\n");
		g_hprmRemote = 0;
		g_hprmHeadphone = 0;
		if (g_motionexists)
		{
			//sceKernelReleaseIntrHandler(36);
			sioClose();
			sceHprmInit();
			//printf("Motion kit removed.\n");
			initaccelbuffer();
			DEBUG_PRINTF("Motion kit removed.\n");
			g_motionexists = 0;
		}
	}
}

void RegisterCallbacks()
{
	sceSysconSetHPConnectCallback(HPConnectCallback,0);
	sceSysconSetHRPowerCallback(HRPowerCallback,0);
}

void checkHeadphoneRemote()
{
	if (g_motionexists) return;

	if (g_hprmRemote!=sceHprmIsRemoteExist())
	{
		printf("--REMOTE EVENT-- ");
		if (!(g_hprmRemote = sceHprmIsRemoteExist()))
		{
			printf("Remote plugged out.\n");
		}
		else
		{
			printf("Remote plugged in.\n");
		}
	}
	
	if (g_hprmHeadphone!=sceHprmIsHeadphoneExist())
	{
		printf("--HEADPHONE EVENT-- ");
		if (!(g_hprmHeadphone = sceHprmIsHeadphoneExist()))
		{
			printf("Headphone plugged out.\n");
			if (!g_muted) sceCodecOutputEnable(0,1);
			if (g_motionexists)
			{
				printf("Motion kit removed.\n");
				initaccelbuffer();
				DEBUG_PRINTF("Motion kit removed.\n");
				g_motionexists = 0;
			}
		}
		else
		{
			printf("Headphone plugged in.\n");
			int wait = 1000;
			while (!sceHprmIsRemoteExist()&&wait>0)
			{
				sceKernelDelayThread(1000);
				wait--;
			}
			if (!sceHprmIsRemoteExist())
			{
				//int intr = sceKernelCpuSuspendIntr();
				sceKernelDisableIntr(36);
				sceSysregUartIoEnable(4);
				sceSysconCtrlHRPower(1);
				hprmResetUART(510416);
				sceKernelDelayThread(25000);
				sioPutchar(0x31);
				int ret = sioWaitGetchar();
				printf("SIO: %i\n", ret);
				if (ret>32 && ret<(255-32))
				{
					// There is a SIO connection from motion kit (most likely)
					sceHprmEnd();
					sceKernelDelayThread(100000);
					sceSysregUartIoEnable(4);
					sceSysconCtrlHRPower(1);
					hprmResetUART(510416);
					RegisterCallbacks();
					if ((g_muted=sceImposeGetParam(PSP_IMPOSE_MUTE))==0)
						sceCodecOutputEnable(0,1);
					g_motionexists = 1;
					g_motioninputavailable = 0;
					initaccelbuffer();
					printf("Motion kit detected.\n");
					DEBUG_PRINTF("Motion kit detected.\n");
					sceKernelDelayThread(1000000);
				}
				else
				{
					if ((g_muted=sceImposeGetParam(PSP_IMPOSE_MUTE))==0)
						sceCodecOutputEnable(1,0);
					g_motionexists = 0;
					hprmResetUART(4800);
					//sceKernelCpuResumeIntr(intr);
				}
				//hprmResetUART(4800);
				sceKernelEnableIntr(36);
			}
		}
	}
}

int filter_accel( int n, float d )
{
	if (n<=0) return g_accelbuffer[g_accelindex].value;
	float temp[3] = {0.f,0.f,0.f};
	int index = g_accelindex - n;
	if (index<0) index += ACCELBUFFER_SIZE;
	int i = 0;
	float factor = 0.f;
	float dd = 1.f;
	while (i++<n)
	{
		factor += dd;
		dd *= d;
		temp[0] *= d;
		temp[1] *= d;
		temp[2] *= d;
		temp[0] += (float)(g_accelbuffer[index].x - 128);
		temp[1] += (float)(g_accelbuffer[index].y - 128);
		temp[2] += (float)(g_accelbuffer[index].z - 128);
		
		index = (index+1)%ACCELBUFFER_SIZE;
	}
	return (((int)(temp[0]/factor) + 128) | (((int)(temp[1]/factor) + 128)<<8) | (((int)(temp[2]/factor) + 128)<<16));
}


#define VAL128_PI 40.743665431525205956834243423364f
#define PI_2 1.5707963267948966192313216916398f

void calc_rotation( int x, int y, int z )
{
	// Clamp values - prevents rotation to overreact on acceleration pushes
	x = x > 32 ? 32 : (x < -32 ? -32 : x);
	y = y > 32 ? 32 : (y < -32 ? -32 : y);
	z = z > 32 ? 32 : (z < -32 ? -32 : z);
	float fx = (float)x / 32.f;
	float fy = (float)y / 32.f;
	float fz = (float)z / 32.f;

	int rz = (int)((atan2f( (float)-fy, (float)fx )-PI_2) * VAL128_PI);
	if (rz<0) rz += 256;
	g_currot.z = (unsigned char)(rz & 0xFF);

	int rx = (int)((atan2f( (float)-fy, (float)-fz )-PI_2) * VAL128_PI);
	if (rx<0) rx += 256;
	g_currot.x = (unsigned char)(rx & 0xFF);

	int ry = (int)((atan2f( (float)fz, (float)fx )) * VAL128_PI);
	if (ry<0) ry += 256;
	g_currot.y = (unsigned char)(ry & 0xFF);
}

void update_accel()
{
	//g_lastaccel = g_curaccel;
	g_accelindex = (g_accelindex+1)%ACCELBUFFER_SIZE;
	if (!(g_motionenabled&g_motionexists))
	{
		g_accelbuffer[g_accelindex].value = 0x80808080;
		g_nullaccel.value = filter_accel( 15, 1.0f );
		g_curaccel.value = filter_accel( g_filterwidth, g_filterweight );
		g_currot.value = 0x80808080;
		g_motioninputavailable = 0;
		g_motioninputs = 0;
		return;
	}
	int intr = sceKernelCpuSuspendIntr();
	
	// Force power on, because sceHprm_driver disables those if it can't find a remote connected
	//sceKernelDisableIntr(36);	
	sceSysregUartIoEnable(4);
	sceSysconCtrlHRPower(1);
	hprmResetUART(510416);
	sioSetTimeout(5000);

	unsigned int value = 0;
	int i = 0;
	if (g_motioninputs<ACCELBUFFER_SIZE)
		g_motioninputs++;
	g_motioninputavailable = g_motioninputs>g_filterwidth;
	while (i++<3)
	{
		value <<= 8;
		int accel = -2;
		if (sioPutchar(0x34-i)>=0)
			accel = sioWaitGetchar();
		if (accel>=0)
		{
			if (g_filterpow>1.f)
			{
				float f = (float)((accel & 0xFF) - 128) / 128.f;
				f = (f<0?-128.f:128.f) * powf( fabsf(f), g_filterpow );
				accel = CLAMP127((int)f) + 128;
			}
			value |= (accel & 0xFF);
		}
		else
		{
			value |= 0x80;
			g_motioninputs = 0;
			g_motioninputavailable = 0;
		}
	}
	g_accelbuffer[g_accelindex].value = value;
	
	g_nullaccel.value = filter_accel( 15, 1.f );
	g_curaccel.value = filter_accel( g_filterwidth, g_filterweight );
	calc_rotation( g_accelbuffer[g_accelindex].x-128, g_accelbuffer[g_accelindex].y-128, g_accelbuffer[g_accelindex].z-128 );

	{
		int x = g_curaccel.x - (g_nullaccel.x - 128);
		g_curaccel.x = (x<0?0:x>255?255:x);
		int y = g_curaccel.y - (g_nullaccel.y - 128);
		g_curaccel.y = (y<0?0:y>255?255:y);
		int z = g_curaccel.z - (g_nullaccel.z - 128);
		g_curaccel.z = (z<0?0:z>255?255:z);
	}
	//DEBUG_RESET()
	//DEBUG_PRINTF("ACCEL: %i, %i, %i [0x%08X]\n", g_curaccel.x-128, g_curaccel.y-128, g_curaccel.z-128, g_curaccel.value )
	//printf("ACCEL: %i, %i, %i (%i) [0x%08X]\n", g_curaccel.x-128, g_curaccel.y-128, g_curaccel.z-128, g_curaccel.pad-128, g_curaccel.value );
	//calc_rotation( g_curaccel.x-128, g_curaccel.y-128, g_curaccel.z-128 );
	//g_currot.value = 0x808080;
	//printf("ROT: %i, %i, %i [0x%08X]\n", g_currot.x - 128,g_currot.y - 128,g_currot.z - 128,g_currot.value);

	//hprmResetUART(4800);
	//sceKernelEnableIntr(36);
	sceKernelCpuResumeIntr(intr);

	//printf("ACCEL: %i, %i, %i (%i) [0x%08X]\n", g_curaccel.x-128, g_curaccel.y-128, g_curaccel.z-128, g_curaccel.pad-128, g_curaccel.value );
	//printf("ROT  : %i, %i, %i (%i) [0x%08X]\n", g_currot.x - 128, g_currot.y - 128, g_currot.z - 128, g_currot.pad - 128, g_currot.value );
}



/* Power Callback */
int power_callback(int unknown, int pwrflags, void *common)
{
    /* check for power switch and suspending as one is manual and the other automatic */
    if (pwrflags & PSP_POWER_CB_POWER_SWITCH || pwrflags & PSP_POWER_CB_SUSPENDING) {
		if (g_debug) DEBUG_PRINTF("GOING TO SUSPEND!\n");
    } else if (pwrflags & PSP_POWER_CB_RESUMING) {
		if (g_debug) DEBUG_PRINTF("RESUMING FROM SUSPEND!\n");
		// Check if motion kit is still plugged in
		//if (g_motionexists)
		{
			g_motionexists = 0;
			g_hprmRemote = -1;
			g_hprmHeadphone = -1;
			sceKernelDelayThread(1000000);
			sceHprmInit();
			return 0;
			/*sceHprmEnd();
			sceKernelDisableIntr(36);
			sceSysregUartIoEnable(4);
			sceSysconCtrlHRPower(1);
			hprmResetUART(510416);
			sceKernelDelayThread(25000);
			sioPutchar(0x31);
			int ret = sioWaitGetchar();
			sceKernelEnableIntr(36);
			if (ret>32 && ret<(255-32))
			{
				RegisterCallbacks();
				if ((g_muted=sceImposeGetParam(PSP_IMPOSE_MUTE))==0)
					sceCodecOutputEnable(0,1);
				if (g_debug) DEBUG_PRINTF("RESUMING FROM SUSPEND! SIO: %i\nMotion kit detected!\n", ret);
			}
			else
			{
				g_motionexists = 0;
				sceHprmInit();
				if (g_debug) DEBUG_PRINTF("RESUMING FROM SUSPEND! SIO: %i\nMotion kit removed!\n", ret);
			}*/
		}
    } else if (pwrflags & PSP_POWER_CB_STANDBY) {
    
    }
	return 0;
}

int main_thread(SceSize args, void *argp)
{
	sceKernelDelayThread(200000);
	printf("Hooking ctrl functions...\n");
	int ret = 0;
	ret |= hook_function( (unsigned int*) sceCtrlReadBufferPositive, ctrl_hook_func, &g_ctrl_common );
	ret |= hook_function( (unsigned int*) sceCtrlPeekBufferPositive, ctrl_hook_func, &g_ctrl_common );
	ret |= hook_function( (unsigned int*) sceCtrlReadBufferNegative, ctrl_hook_func, &g_ctrl_common );
	ret |= hook_function( (unsigned int*) sceCtrlPeekBufferNegative, ctrl_hook_func, &g_ctrl_common );
	if (ret)
	{
		printf("Could not hook controller functions\n");
	}

	printf("\nHooking sceDisplaySetFrameBuf...\n");
	ret = hook_function( (unsigned int*) sceDisplaySetFrameBuf, setframebuf_hook_func, &g_setframebuf );
	if (ret)
	{
		printf("Could not hook setframebuf function\n");
	}

	sceKernelDcacheWritebackInvalidateAll();
	sceKernelIcacheInvalidateAll();

	DEBUG_PRINTF("Launching: %s\n", g_executable);

	g_muted = sceImposeGetParam(PSP_IMPOSE_MUTE);
	//sceHprmInit();
	while (!g_quit)
	{
		checkHeadphoneRemote();
		if (g_motionexists || g_hprmHeadphone==0)
			if (g_muted!=sceImposeGetParam(PSP_IMPOSE_MUTE) || resetoutput)
			{
				g_muted = sceImposeGetParam(PSP_IMPOSE_MUTE);
				if (g_muted)
					sceCodecOutputEnable(0,0);
				else
					sceCodecOutputEnable(0,1);
				if (resetoutput) resetoutput--;
			}
		//if (g_motionexists && g_motionenabled) sceSysconCtrlHRPower(1);
		sceKernelDelayThread(g_samplingcycles);
		update_accel();
		if (g_savesettings)
		{
			g_savesettings = 0;
			save_config();
		}
	}


	//printf("Unhooking functions...");
	//unhook_function( (unsigned int*) sceDisplaySetFrameBuf, (unsigned int)g_setframebuf );
	//unhook_function( (unsigned int*) sceCtrlReadBufferPositive, (unsigned int)g_ctrl_common );
	//printf("Done.\n");
	return 0;
}


int power_thread(SceSize args, void *argp)
{
	int cbid = sceKernelCreateCallback("motion_driver Power Callback", power_callback, NULL);
	scePowerRegisterCallback(0, cbid);
	sceKernelSleepThreadCB();

	return 0;
}

/*
enum PSPInitApitype
{
	PSP_INIT_APITYPE_DISC = 0x120,
	PSP_INIT_APITYPE_DISC_UPDATER = 0x121,
	PSP_INIT_APITYPE_MS1 = 0x140,
	PSP_INIT_APITYPE_MS2 = 0x141,
	PSP_INIT_APITYPE_MS3 = 0x142,
	PSP_INIT_APITYPE_MS4 = 0x143,
	PSP_INIT_APITYPE_MS5 = 0x144,
	PSP_INIT_APITYPE_VSH1 = 0x210, / ExitGame /
	PSP_INIT_APITYPE_VSH2 = 0x220, / ExitVSH /
};
*/

/* Entry point */
int module_start(SceSize args, void *argp)
{
	int thid;

	//sceIoRemove("ms0:/SEPLUGINS/motion_drv.log");

	printf("-----------------------------------------------\n");
	printf("PLUGIN ENTRY POINT - module_start\n");
	printf("apitype: %x\n", sceKernelInitApitype() );
	printf("bootfrom: %x\n", sceKernelBootFrom() );
	printf("keyconfig: %x\n", sceKernelInitKeyConfig() );
	printf("executable: %s\n", sceKernelInitFileName() );

	/*if (sceKernelInitApitype()==0x143)
	{
		strcpy(g_stringbuffer,"POPS");
		g_executable = g_stringbuffer;
	}
	else*/
	if (sceKernelInitApitype()>=0x210)
	{
		strcpy(g_stringbuffer,"VSH");
		g_executable = g_stringbuffer;
	}
	else
	if (sceKernelInitApitype()==0x120)
	{
		// If UMD present
		if (sceUmdCheckMedium(0))
		{
			// Mount UMD to disc0: file system
			sceUmdActivate(1,"disc0:");
			
			// Wait for init
			sceUmdWaitDriveStat(UMD_WAITFORINIT);

			// Find umd id string
			int fdUMD = sceIoOpen("disc0:/UMD_DATA.BIN",PSP_O_RDONLY,0);
			char umdID[11];
			if (fdUMD >= 0)
			{
				// Read id
				sceIoRead(fdUMD,umdID,10);
				
				// End string
				umdID[10] = 0;
				
				// Close file
				sceIoClose(fdUMD);
			}
			else
				strcpy(umdID,"unknown");
			
			sceUmdDeactivate(1,"disc0:");

			printf("UMD ID: %s\n", umdID);
			
			g_executable = g_stringbuffer;
			strcpy(g_executable, umdID);
		}
	}
	else
	{
		g_executable = sceKernelInitFileName();
	}

	if (sceKernelInitApitype()==0x100 || sceKernelInitApitype()==0x121 || sceKernelInitApitype()==0x140 || strstr(g_executable,"recovery.prx")!=0)
	{
		printf("Plugin stopped in recovery/updater mode\n");
		return 0;
	}
	
	printf("\n<-------------------------------------------------------------------->");
	printf("\nCreating thread...");
	thid = sceKernelCreateThread("motion_driver_power", power_thread, 0x11, 1*1024, 0, NULL);
	if (thid >= 0)
		sceKernelStartThread(thid, 0, 0);
	
	thid = sceKernelCreateThread("motion_driver_main", main_thread, 0x20, 4*1024, 0, NULL);
	if(thid >= 0)
	{
		printf("success.\n");
		sceKernelStartThread(thid, args, argp);
		sceKernelDeleteThread(thid);
	}
	else
		printf("failed!\n");

	
	printf("\nLoading configuration...\n");
	load_config();
	printf("\nDone.\n");
	
	return 0;
}

/* Module stop entry */
int module_stop(SceSize args, void *argp)
{
	printf("\nStopping module...\n");
	g_quit = 1;
	return 0;
}


int module_reboot_before(SceSize args, void *argp)
{
	printf("\nReboot module...\n");
	g_quit = 1;
	return 0;
}




// DRIVER EXPORTS

int motionGetDriverVersion()
{
	return g_version;
}

int motionEnable()
{
	g_motionenabled = 1;
	return 0;
}

int motionDisable()
{
	g_motionenabled = 0;
	return 0;
}

int motionDisableForward()
{
	memset( g_buttons.Buttons, 0, sizeof(g_buttons) );
	g_motionforward = 0;
	return 0;
}

int motionSetForward( int axis, int buttons )
{
	if ((axis >> AXIS_SHIFT)>5) return -1;
	g_buttons.Buttons[(axis >> AXIS_SHIFT)][axis&1] = buttons;
	g_motionforward = 1;
	return 0;
}

int motionGetForward( int axis )
{
	if ((axis >> AXIS_SHIFT)>5) return 0;
	return g_buttons.Buttons[(axis >> AXIS_SHIFT)][axis&1];
}

int motionSetFilter( int n, float weight )
{
	g_filterwidth = (n>15?15:n<0?0:n);
	g_filterweight = (weight<0.05f?0.05f:weight);
	return 0;
}

int motionSetPow( float pow )
{
	if (pow < 1.f) pow = 1.f;
	if (pow > 3.f) pow = 3.f;
	g_filterpow = pow;
	return 0;
}

int motionGetAccel( motionAccelData* accel )
{
	if (accel==0)
		return -1;
	accel->value = g_accelbuffer[g_accelindex].value;
	return 0;
}

int motionGetRelAccel( motionAccelData* accel )
{
	if (accel==0)
		return -1;

	accel->value = g_curaccel.value;
	/*accel->x = ((g_curaccel.x - g_lastaccel.x)/2) + 128;
	accel->y = ((g_curaccel.y - g_lastaccel.y)/2) + 128;
	accel->z = ((g_curaccel.z - g_lastaccel.z)/2) + 128;*/
	return 0;
}

int motionGetRotation( motionAccelData* accel )
{
	if (accel==0)
		return -1;
	
	accel->value = g_currot.value;
	return 0;
}

int motionExists()
{
	return g_motionexists && g_motioninputavailable;
}

int motionEnabled()
{
	return g_motionenabled;
}

int motionSetSampling( int n )
{
	if (n<1) n = 20;	// default
	if (n>60) n = 60;
	g_samplingcycles = (1000/n)*1000;
	return 0;
}

int motionGetSampling()
{
	return 1000/(g_samplingcycles/1000);
}

char* motionDriverVersionString()
{
	static char str[64];
	sprintf("Version %x.%02x.%02x%x compiled on %s %s", (g_version >> 24) & 0xFF, (g_version >> 16) & 0xFF, (g_version >> 8) & 0xFF, (g_version & 0xFF), __DATE__, __TIME__);
	return str;
}

