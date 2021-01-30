#include <stdio.h>
#ifdef RELEASE
#define printf(...)
// 	{ char temp[2]; sprintf(temp," "); }
#else
extern void dbgfileprint( char* msg );
#define printf(...) { char temp[512]; sprintf(temp,__VA_ARGS__); dbgfileprint(temp); }
#endif
