#include <pspiofilemgr.h>
#include <string.h>
#include "config.h"
#include "utils.h"
#include "debug.h"


#define BUFFER_SIZE 512
#define LINE_SIZE 255
char* readLine( int fd )
{
	static char buffer[BUFFER_SIZE];
	static int bufferSize = 0;
	static int bufferPos = 0;
	static char line[LINE_SIZE+1];
	int i = 0;
	do
	{
		if (bufferSize==0)
		{
			bufferPos = 0;
			bufferSize = sceIoRead( fd, buffer, BUFFER_SIZE );
			if (bufferSize==0)
				return 0;
		}
		while (bufferSize>0 && i<LINE_SIZE && buffer[bufferPos]!='\n')
		{
			line[i++] = buffer[bufferPos++];
			bufferSize--;
		}
	}
	while (i<LINE_SIZE && buffer[bufferPos]!='\n');

	if (buffer[bufferPos]=='\n')
	{
		bufferPos++;
		bufferSize--;
	}
	//if (line[i-1]=='\n') line[i-1]=0;
	line[i] = 0;
	return line;
}


void cfgReadCallback( const char* filename, char* section, void (*callback)( char*, char* ) )
{
	if (!callback) return;
	
	int fd = sceIoOpen( filename, PSP_O_RDONLY, 0777 );
	if (fd<0)
	{
		printf("Error opening config file '%s'.\n", filename);
		return;
	}
	
	int insection = 0;
	if (!section) insection = 1;

	char* cfg;
	while ((cfg=readLine(fd)))
	{
		while ((*cfg==' ' || *cfg=='\t') && *cfg!=0)
			cfg++;

		if (*cfg==0 || *cfg==';' || *cfg=='#' || (*cfg=='/' && cfg[1]=='/'))
			continue;

		if (*cfg=='[')
		{
			cfg++;
			int n = 0;
			while (cfg[n]!=']' && cfg[n]!=0)
				n++;
			if (n==0)
				continue;

			if (!section || strncmpupr( section, cfg, n )==0)
				insection = 1;
			else
				insection = 0;
		}
		else
		{
			if (!insection)
				continue;
			if (strpbrk( cfg, "=" )==0)
				continue;
			char* element = strtok( cfg, "=" );
			char* value = strtok( NULL, ";#\n\r" );
			if (element && value)
			{
				element = strtok( element, " \t" );	// cut at whitespace
				char* quote = 0;
				if ((quote=strpbrk( value, "\"" ))!=0)
				{
					value = quote+1;
					value = strtok( value, "\"" );	// Tokenize quoted string
				}
				else
				{
					value = strtok( value, " \t" ); // cut at whitespace
				}
				
				callback( element, value );
			}
		}
	}
	
	sceIoClose( fd );
}

/*
cfgFile* cfgRead( const char* filename )
{
	cfgFile* file = malloc(sizeof(cfgFile));
	if (!file) return 0;
	
	int fd = sceIoOpen( filename, PSP_O_RDONLY, 0777 );
	if (fd<0)
	{
		printf("Error opening config file '%s'.\n", filename);
		return 0;
	}

	file->name = malloc(strlen(filename)+1);
	strcpy(file->name, filename);

	char* cfg = 0;
	cfgElement* lastElement = 0;
	while (cfg=readLine(fd))
	{
		while ((*cfg==' ' || *cfg=='\t') && *cfg!=0)
			cfg++;
		
		if (*cfg==0 || *cfg==';' || *cfg=='#' || (*cfg=='/' && cfg[1]=='/'))
			continue;

		if (*cfg=='[')
		{
			cfg++;
			int n = 0;
			while (cfg[n]!=']' && cfg[n]!=0)
				n++;
			if (n==0)
				continue;

			cfgElement* elem = malloc(sizeof(cfgElement)+n);
			elem->name = (char*)elem+sizeof(cfgElement);
			strncpy(elem->name,&cfg[1],n-1);
			elem->name[n-1] = 0;
			elem->next = 0;
			elem->svalue = 0;
			elem->value = 0.f;
			if (lastElement)
				lastElement->next = elem;
			else
				file->elements = elem;
			lastElement = elem;
		}
		else
		{
			if (strpbrk( cfg, "=" )==0)
				continue;
			char* element = strtok( cfg, "=" );
			char* value = strtok( NULL, ";#" );
			if (element && value)
			{
				element = strtok( element, " \t" );	// cut at whitespace
				if (strpbrk( value, "\"" )!=0)
				{
					while (*value==' ' || *value=='\t' || *value!=0)
						value++;
					value = strtok( value, "\"" );	// Tokenize quoted string
				}
				else
				{
					value = strtok( value, " \t" ); // cut at whitespace
				}
				
				cfgElement* elem = malloc(sizeof(cfgElement)+strlen(element)+1+strlen(value)+1);
				elem->name = (char*)elem + sizeof(cfgElement);
				strcpy( elem->name, element );
				elem->svalue = (char*)elem + sizeof(cfgElement)+strlen(element)+1;
				strcpy( elem->svalue, value );
				elem->value = atof( element );
				elem->next = 0;
				
				if (lastElement)
					lastElement->next = elem;
				else
					file->elements = elem;
				lastElement = elem;
			}
		}
	}
}


int cfgWrite( cfgFile* cfg )
{
	return 0;
}

void cfgClose( cfgFile* cfg )
{
	if (cfg==0)
		return;
	
	cfgElement* elem = cfg->elements;
	cfgElement* next = 0;
	while(elem)
	{
		//if (elem->name) free(elem->name);
		//if (elem->svalue) free(elem->svalue);
		next = elem->next;
		free(elem);
		elem = next;
	}
	free(cfg);
}

char*	cfgGetString( cfgFile* cfg, const char* element, const char* default_value )
{
	if (!cfg)
		return default_value;
	char* string = alloca(strlen(element)+1);
	strcpy( string, element );
	char* section = strtok( string, "." );
	char* name = strtok( NULL, " \t" );
	if (!section || !name)
		return default_value;
	
	cfgElement* elem = cfg->elements;
	int inSection = 0;
	while(elem)
	{
		if (!inSection)
		{
			if (elem->svalue==0 && strcmp( elem->name, section )==0)
			{
				inSection = 1;
			}
		}
		else
		{
			if (elem->svalue==0)
			{
				// We got out of the section scope, so there is no match
				return default_value;
			}
			if (strcmp( elem->name, name )==0)
				return elem->svalue;
		}
		elem = elem->next;
	}
	return default_value;
}

float	cfgGetFloat( cfgFile* cfg, const char* element, float default_value )
{
	if (!cfg)
		return default_value;
	char* string = alloca(strlen(element)+1);
	strcpy( string, element );
	char* section = strtok( string, "." );
	char* name = strtok( NULL, " \t" );
	if (!section || !name)
		return default_value;
	
	cfgElement* elem = cfg->elements;
	int inSection = 0;
	while(elem)
	{
		if (!inSection)
		{
			if (elem->svalue==0 && strcmp( elem->name, section )==0)
			{
				inSection = 1;
			}
		}
		else
		{
			if (elem->svalue==0)
			{
				// We got out of the section scope, so there is no match
				return default_value;
			}
			if (strcmp( elem->name, name )==0)
				return elem->value;
		}
		elem = elem->next;
	}
	return default_value;
}

int		cfgGetInt( cfgFile* cfg, const char* element, int default_value )
{
	return (int)cfgGetFloat( cfg, element, (float)default_value );
}


int		cfgSetElementString( cfgFile* cfg, const char* element, char* value )
{
}

int		cfgSetElementFloat( cfgFile* cfg, const char* element, float value )
{
}

int		cfgSetElementInt( cfgFile* cfg, const char* element, int value )
{
}
*/
