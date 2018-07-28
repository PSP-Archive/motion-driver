#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <motion.h>

PSP_MODULE_INFO("Motion kit sample",  0x0000, 1, 1);
PSP_MAIN_THREAD_ATTR(0);

int running = 1;
/* Exit callback */
int exit_callback(int arg1, int arg2, void *common) {
	running = 0;
	return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp) {
	int cbid;
	
	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	
	return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void) {
	int thid = 0;
	
	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0) {
			sceKernelStartThread(thid, 0, 0);
	}
	
	return thid;
}


int Xaccel = 0;
int Yaccel = 0;
int Zaccel = 0;

int main() {
	SetupCallbacks();
	pspDebugScreenInit();

	// Load the motion kit driver
	int ret = motionLoad();
	pspDebugScreenPrintf("Loading motion driver... [0x%08X]\n", ret);
	if (ret < 0)
	{
		pspDebugScreenPrintf("Error! Driver could not be loaded!\n");
		pspDebugScreenPrintf("Press X to exit.\n");
		while(running)
		{
			SceCtrlData pad;
			sceCtrlReadBufferPositive(&pad, 1);
			if(pad.Buttons & PSP_CTRL_CROSS)
			{
				break;
			}
			sceKernelDelayThread(10000);
		}
		sceKernelExitGame();
		return 0;
	}

	pspDebugScreenPrintf("Motion kit driver version: 0x%08x\n", motionGetDriverVersion());
	
	pspDebugScreenPrintf("Press X to exit.\n");
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	// Disable button forwarding to avoid a movement to press Cross and quit the program
	motionDisableForward();
	while(running)
	{
		SceCtrlData pad;
		sceCtrlReadBufferPositive(&pad, 1);
		if(pad.Buttons & PSP_CTRL_CROSS)
		{
			break;
		}

		if (!motionExists())
		{
			pspDebugScreenSetXY(0,3);
			pspDebugScreenPrintf("Please plug in the motion kit...\n\n\n");
			while (!motionExists() && running)
			{
				SceCtrlData pad;
				sceCtrlReadBufferPositive(&pad, 1);
				if(pad.Buttons & PSP_CTRL_CROSS)
				{
					break;
				}
				sceKernelDelayThread(10000);
			}
			if (motionExists())
				pspDebugScreenPrintf("Motion kit plugged in...\n");
		}
		
		sceKernelDelayThread(10000);
		
		motionAccelData accel;
		motionGetAccel( &accel );
		Xaccel = accel.x - 128;
		Yaccel = accel.y - 128;
		Zaccel = accel.z - 128;
		
		pspDebugScreenSetXY(0,3);
		pspDebugScreenPrintf( "Accel   : (%i, %i, %i)        \n", Xaccel, Yaccel, Zaccel );
		
		motionGetRelAccel( &accel );
		Xaccel = accel.x - 128;
		Yaccel = accel.y - 128;
		Zaccel = accel.z - 128;
		pspDebugScreenPrintf( "RelAccel: (%i, %i, %i)        \n", Xaccel, Yaccel, Zaccel );
		
		motionGetRotation( &accel );
		Xaccel = accel.x - 128;
		Yaccel = accel.y - 128;
		Zaccel = accel.z - 128;
		pspDebugScreenPrintf( "Rotation: (%i, %i, %i)        \n", Xaccel, Yaccel, Zaccel );
	}

	sceKernelExitGame();
	return 0;
}
