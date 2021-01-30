#ifndef __CONFIG_H__
#define __CONFIG_H__


typedef struct cfgElement
{
	char*		name;
	char*		svalue;
	float		value;

	struct cfgElement*	next;
} cfgElement;


typedef struct cfgFile
{
	char*		name;
	cfgElement*	elements;
} cfgFile;


void cfgReadCallback( const char* filename, char* section, void (*callback)( char*, char* ) );

cfgFile*	cfgRead( const char* filename );
int			cfgWrite( cfgFile* cfg );
void		cfgClose( cfgFile* cfg );


char*	cfgGetString( cfgFile* cfg, const char* element, const char* default_value );
float	cfgGetFloat( cfgFile* cfg, const char* element, float default_value );
int		cfgGetInt( cfgFile* cfg, const char* element, int default_value );

int		cfgSetString( cfgFile* cfg, const char* element, char* value );
int		cfgSetFloat( cfgFile* cfg, const char* element, float value );
int		cfgSetInt( cfgFile* cfg, const char* element, int value );

#endif
