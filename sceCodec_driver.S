	.set noreorder

#include "pspimport.s"

// Build List
// sceCodec_driver_0000.o sceCodec_driver_0001.o sceCodec_driver_0002.o sceCodec_driver_0003.o sceCodec_driver_0004.o sceCodec_driver_0005.o sceCodec_driver_0006.o sceCodec_driver_0007.o sceCodec_driver_0008.o sceCodec_driver_0009.o sceCodec_driver_0010.o sceCodec_driver_0011.o sceCodec_driver_0012.o sceCodec_driver_0013.o sceCodec_driver_0014.o 

	IMPORT_START	"sceCodec_driver",0x00010011
	IMPORT_FUNC	"sceCodec_driver",0x02133959,sceCodecStopEntry
	IMPORT_FUNC	"sceCodec_driver",0x20C61103,sceCodec_driver_20C61103
	IMPORT_FUNC	"sceCodec_driver",0x261C6EE8,sceCodecSetOutputVolume
	IMPORT_FUNC	"sceCodec_driver",0x28A49CCA,sceCodec_driver_28A49CCA
	IMPORT_FUNC	"sceCodec_driver",0x3064C53D,sceCodec_driver_3064C53D
	IMPORT_FUNC	"sceCodec_driver",0x31B2E41E,sceCodecInputDisable
	IMPORT_FUNC	"sceCodec_driver",0x359C2B9F,sceCodecOutputDisable
	IMPORT_FUNC	"sceCodec_driver",0x40D5C897,sceCodec_driver_40D5C897
	IMPORT_FUNC	"sceCodec_driver",0x6D945509,sceCodecNotifyAvcKick
	IMPORT_FUNC	"sceCodec_driver",0x856E7487,sceCodecOutputEnable
	IMPORT_FUNC	"sceCodec_driver",0xBD8E0977,sceCodecInitEntry
	IMPORT_FUNC	"sceCodec_driver",0xC513C747,sceCodecInputEnable
	IMPORT_FUNC	"sceCodec_driver",0xDFBCACF3,sceCodecSetFrequency
	IMPORT_FUNC	"sceCodec_driver",0xEEB91526,sceCodecSetVolumeOffset
