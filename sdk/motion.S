	.set noreorder

#include "pspimport.s"

	IMPORT_START "motion_driver",0xC0090000
	IMPORT_FUNC  "motion_driver",0xEB4A30BC,motionGetDriverVersion
	IMPORT_FUNC  "motion_driver",0x50FA90AC,motionEnable
	IMPORT_FUNC  "motion_driver",0xA33941D6,motionDisable
	IMPORT_FUNC  "motion_driver",0x4B7C4DEC,motionDisableForward
	IMPORT_FUNC  "motion_driver",0x9523DD23,motionSetForward
	IMPORT_FUNC  "motion_driver",0xEE6B06A6,motionGetForward
	IMPORT_FUNC  "motion_driver",0x597EC13C,motionSetFilter
	IMPORT_FUNC  "motion_driver",0x714826B3,motionGetAccel
	IMPORT_FUNC  "motion_driver",0x42109DA9,motionGetRelAccel
	IMPORT_FUNC  "motion_driver",0xE97D21B8,motionGetRotation
	IMPORT_FUNC  "motion_driver",0xE0480016,motionExists
	IMPORT_FUNC  "motion_driver",0x991F7996,motionEnabled
	IMPORT_FUNC  "motion_driver",0x3935F08D,motionSetSampling
	IMPORT_FUNC  "motion_driver",0xC3CF0383,motionGetSampling
