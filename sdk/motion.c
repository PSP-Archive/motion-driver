#include <pspkernel.h>
#include <string.h>

static SceUID load_module(const char *path, int flags, int type)
{
	SceKernelLMOption option;
	SceUID mpid;

	/* If the type is 0, then load the module in the kernel partition, otherwise load it
	   in the user partition. */
	if (type == 0) {
		mpid = 1;
	} else {
		mpid = 2;
	}

	memset(&option, 0, sizeof(option));
	option.size = sizeof(option);
	option.mpidtext = mpid;
	option.mpiddata = mpid;
	option.position = 0;
	option.access = 1;

	return sceKernelLoadModule(path, flags, type > 0 ? &option : NULL);
}


static SceUID load_module_uid(SceUID fd, int flags, int type)
{
	SceKernelLMOption option;
	SceUID mpid;

	/* If the type is 0, then load the module in the kernel partition, otherwise load it
	   in the user partition. */
	if (type == 0) {
		mpid = 1;
	} else {
		mpid = 2;
	}

	memset(&option, 0, sizeof(option));
	option.size = sizeof(option);
	option.mpidtext = mpid;
	option.mpiddata = mpid;
	option.position = 0;
	option.access = 1;

	return sceKernelLoadModuleByID(fd, flags, type > 0 ? &option : NULL);
}

int motionLoad()
{
	SceUID modid = -1;
	
	SceUID fd = sceIoOpen( "motion_driver.prx", PSP_O_RDONLY, 0777);
	if (fd<0)
	{
		return -2;
	}
	
	if ((modid=load_module_uid( fd, 0, 0 ))>=0)
	{
		sceIoClose(fd);
	}
	
	if (modid==0x80020139)
		return 0;
	
	if (modid<0)
	{
		modid = load_module("motion_driver.prx", 0, 0);
	}
	
	if (modid>=0)
	{
		int fd;
		return sceKernelStartModule(modid, 0, NULL, &fd, NULL);
	}
	return modid;
}
