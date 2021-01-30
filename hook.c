#include <pspsdk.h>

#define GET_JUMP_TARGET(x) (0x80000000 | (((x) & 0x03FFFFFF) << 2))

int hook_function(unsigned int* jump, void* hook, unsigned int* result)
{
	unsigned int target;
	unsigned int func;
	int inst;

	target = GET_JUMP_TARGET(*jump);
	while (((inst = _lw(target+4)) & ~0x03FFFFFF) != 0x0C000000)	// search next JAL instruction
		target += 4;

	if((inst & ~0x03FFFFFF) != 0x0C000000)
	{
		return 1;
	}

	*result = GET_JUMP_TARGET(inst);
	func = (unsigned int) hook;
	func = (func & 0x0FFFFFFF) >> 2;
	_sw(0x0C000000 | func, target+4);

	return 0;
}

/*
int unhook_function(unsigned int* jump, unsigned int result)
{
	unsigned int target;
	unsigned int func;
	int inst;

	printf("\nhooking %p\n", jump);
	target = GET_JUMP_TARGET(*jump);
	printf("target %08X\n", target);
	while (((inst = _lw(target+4)) & ~0x03FFFFFF) != 0x0C000000)	// search next JAL instruction
		target += 4;
	printf("target final %08X\n", target+4);
	printf("inst %08X\n", inst);
	if((inst & ~0x03FFFFFF) != 0x0C000000)
	{
		printf("invalid!\n");
		return 1;
	}

	func = (unsigned int) result;
	func = (func & 0x0FFFFFFF) >> 2;
	_sw(0x0C000000 | func, target+4);

	return 0;
}
*/


/*
int hook_setframebufinternal(unsigned int* jump, void* hook, unsigned int* result)
{
	unsigned int target;
	unsigned int func;
	int inst, next;

	printf("\nhooking %p\n", jump);
	target = GET_JUMP_TARGET(*jump);
	printf("target %08X\n", target);
	inst = _lw(target);
	next = _lw(target+4);
	printf("inst %08X\n", inst);
	printf("next %08X\n", next);
	if (inst!=0x27BDFFD0 || next!=0xAFB10004)
	{
		// We're at the wrong address?
		return 1;
	}
	
	*result = (unsigned int*)(target+8);
	func = (unsigned int) hook;
	func = (func & 0x0FFFFFFF) >> 2;
	_sw(0x08000000 | func, target);		// j func
	_sw(inst,target+4);					// delay slot = first real instruction

	// second real instruction is executed in delay slot of hooked functions jr
	return 0;
}
*/
