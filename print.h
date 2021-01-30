

#define DEBUGMSG_SIZE 512
static char debugmsg[DEBUGMSG_SIZE+64];
char* debugcur = debugmsg;
static int debuglines = 0;

extern u8 msx[];

void scrprint(int x, int y, const char* text, void* where, int psm)
{
	if (text==0 || *text==0 || where==0) return;
	int c, i, j, l;
	unsigned char *font;
	unsigned short *vram;
	void *baseptr = 0;

	//sceKernelDcacheWritebackAll();
	baseptr = (void*)((unsigned int)where&~0x40000000);
	if (!((unsigned int)baseptr>=0x4000000 && (unsigned int)baseptr<0x4200000) &&
		!((unsigned int)baseptr>=0x8800000 && (unsigned int)baseptr<0xA000000)) return;

	int start_x = x, start_y = y;
	for (c = 0; c < DEBUGMSG_SIZE; c++) {
		char ch = text[c];
		if (ch==0) break;
		if (ch!='\n')
		{
			vram = (unsigned short*)baseptr + (x + y * 512);
			if (psm==3)
				vram += (x + y * 512);
			
			font = &msx[ (int)ch * 8 ];
			for (i = l = 0; i < 8; i++, l += 8, font++) {
				unsigned short* vram_ptr  = vram;
				for (j = 0; j < 8; j++) {
					if ((*font & (128 >> j)))
					{
						if (psm==3)
						{
							*((unsigned int*)vram_ptr+1) = 0xFFFFFFFF;
							*((unsigned int*)vram_ptr+1+512) = 0;
						}
						else
						{
							*vram_ptr = 0xFFFF;
							*(vram_ptr+1+512) = 0;
						}
					}
					vram_ptr++;
					if (psm==3) vram_ptr++;
				}
				vram += 512;
				if (psm==3)
					vram += 512;
			}
			x += 7;
		}
		else
		{
			x = start_x;
			y += 8;
			if (y>=272-8) y = start_y;
		}
		if (x>=480-7)
		{
			x = start_x;
			y += 8;
			if (y >= 272-8) y = start_y;
		}
	}
	
	sceKernelDcacheWritebackRange(baseptr,((unsigned int)vram-(unsigned int)baseptr));
}

#define DEBUG_RESET() { debugcur = debugmsg; memset(debugmsg,0,DEBUGMSG_SIZE); debuglines = 0; g_info = 0; }
#define DEBUG_PRINTF( ... ) { sprintf( debugcur, __VA_ARGS__ ); g_info = 120; } //{ if (debugcur<debugmsg+DEBUGMSG_SIZE && debuglines<=4) { debugcur += sprintf( debugcur, __VA_ARGS__ ); debuglines++; } else { char* line = strchr( debugmsg, '\n' ); memcpy( debugmsg, line, debugcur-line ); debugcur -= (line-debugmsg); debugcur += sprintf( debugcur, __VA_ARGS__ ); }  g_info = 250; }
