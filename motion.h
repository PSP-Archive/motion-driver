/*
 * motion.h: Header for the motion kit driver SDK
 *
 * Copyright (C) 2007 Alexander Berl 'Raphael' <raphael@fx-world.org>
 *
 * This code is part of the motion kit driver SDK version 1.0b
 */

#ifndef __MOTION_H__
#define __MOTION_H__


#ifdef __cplusplus
extern "C" {
#endif

#define MOTION_SDK_VERSION	0x0100000b

typedef union
{
	struct {
		unsigned char x, y, z;
		unsigned char pad;
	};
	unsigned int	value;
} motionAccelData;


enum motionAxis
{
	MOTION_AXIS_POSITIVE_X = 0x00,
	MOTION_AXIS_NEGATIVE_X = 0x01,
	MOTION_AXIS_POSITIVE_Y = 0x10,
	MOTION_AXIS_NEGATIVE_Y = 0x11,
	MOTION_AXIS_POSITIVE_Z = 0x20,
	MOTION_AXIS_NEGATIVE_Z = 0x21,
	
	// Rotational axes
	MOTION_ROT_POSITIVE_X = 0x30,		// Pitch
	MOTION_ROT_NEGATIVE_X = 0x31,		// Pitch
	MOTION_ROT_POSITIVE_Y = 0x40,		// Yaw
	MOTION_ROT_NEGATIVE_Y = 0x41,		// Yaw
	MOTION_ROT_POSITIVE_Z = 0x50,		// Roll
	MOTION_ROT_NEGATIVE_Z = 0x51,		// Roll
};

// Additional defines for PSP_CTRL masks
#define PSP_CTRL_ANALOG_LEFT	0x10000000
#define PSP_CTRL_ANALOG_RIGHT	0x20000000
#define PSP_CTRL_ANALOG_UP		0x40000000
#define PSP_CTRL_ANALOG_DOWN	0x80000000


/** Load the motion kit driver prx
  * @return < 0 on error, 0 if driver was already loaded or a module ID else
  */
int motionLoad();

/** Get the motion kit driver version
  * @return Driver version in format (0xMMmmrraa, MM = major, mm = minor, rr = revision, aa = additional)
  * @note To compare versions, compare (version >> 8)
  */
int motionGetDriverVersion();

/** Enable the motion kit
  * @return < 0 on error
  */
int motionEnable();

/** Disable the motion kit
  * @return < 0 on error
  */
int motionDisable();


/** Disable the button forwarding feature of the motion kit
  * This is done by setting all buttons forwards to 0, so it can be enabled again
  * by setting new forwards.
  * @return < 0 on error
  */
int motionDisableForward();

/** Set button forwarding for a specific axis
  * @param axis The axis which should get forwarded to a button. One of motionAxis
  * @param buttons The buttons which should receive the actions from the axis. A combination of PSP_CTRL button masks.
  * @return < 0 on error
  */
int motionSetForward( int axis, int buttons );

/** Get the current buttons that receive actions on an axis event
  * @param axis The axis for which to check. One of motionAxis
  * @return The button mask
  */
int motionGetForward( int axis );

/** Set the drivers filter method parameters
  * @param n The number of cycles to filter back (0-15), 0 disables filtering
  * @param weight The weight by which to scale each previous cycle data cumulatively (0.0 - 1.0)
  * @note A weight of 0.5 results in the following filtering (if frame[n] is the current cycle data):
  *       (1.0*frame[n] + 0.5*frame[n-1] + 0.5*0.5*frame[n-2] + ...) / n
  * @return < 0 on error
  */
int motionSetFilter( int n, float weight );

/** Set the drivers acceleration axis power factor
  * @param pow The power to take each acceleration data by to smooth out jumps and inaccuracies
  * @note A pow of 1.f doesn't change the input from the motion kit at all. The higher the value the
  *       less responsive the motion kit will be.
  * @return < 0 on error
  */
int motionSetPow( float pow );


/** Get acceleration data
  * @param accel Pointer to a motionAccelData structure to receive the acceleration values
  * @return < 0 on error
  */
int motionGetAccel( motionAccelData* accel );

/** Get relative acceleration data
  * @param accel Pointer to a motionAccelData structure to receive the acceleration values
  * @note This will only detect changes in acceleration, other than motionGetAccel, which also
  *       measures the constant gravital acceleration.
  * @return < 0 on error
  */
int motionGetRelAccel( motionAccelData* accel );

/** Get rotation (tilt) data
  * @param accel Pointer to a motionAccelData structure to receive the rotation values
  * @note The values are scaled and translated into range 0-255. Therefore a value of 0 translates to -180°,
  *       128 to 0° and 255 translates to 179°.
  *       The axes are aligned as follows: positive x rotation goes towards you, positive y rotation goes to
  *       the back and positive z rotation goes to the right.
  *       Also note, that rotations around the axis parallel to the gravital axis (y if the PSP is held straight in front)
  *       are very imprecise and should not be regarded.
  * @return < 0 on error
  */
int motionGetRotation( motionAccelData* accel );


/** Check if the motion kit is plugged in
  * @return 0 if not plugged in and 1 else
  */
int motionExists();

/** Check if the motion kit is enabled
  * @return 0 if not enabled in and 1 else
  */
int motionEnabled();


/** Set the number of samples to take per second
  * @param n The number of samples per second (max 60, default 30)
  * @return < 0 on error
  */
int motionSetSampling( int n );

/** Get the number of samples taken per second
  * @return Number of samples taken per second
  */
int motionGetSampling();


#ifdef __cplusplus
};
#endif

#endif
