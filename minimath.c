const float shift23=(1<<23);
const float OOshift23=1.0/(1<<23);	// = 1.1920929e-7 = 0.00000011920929

float log2(float i)
{
	const float LogBodge=0.346607f;
	float x;
	float y;
	//Compiler is picky about that line and doesn't compile it reliably into the wanted output
	//x=*(int *)&i;	// i = 0.33 = 00111110101010001111010111000011b = 1051260355 => x = 1051260350.0
	asm("cvt.s.w %0, %1\n"
		:"=f"(x):"f"(i));
	x*= OOshift23; //1/pow(2,23); => x = (float)125.3199999286515 = 125.32
	x=x-127;	// x = -1.68

	y=x-(int)x;		// x - trunc(x) = +-frac(x) => y = -1.68 - (-1) = -0.68
	y=((y<0?-y:y)-y*y)*LogBodge;	// y = (0.68 - 0.4624)*LogBodge = 0.2176*0.346607 = (float)0.0754216832 = 0.07542168
	return x+y;	// return (float)-1.60457832 = -1.6045784 :: log2( 0.33 ) = (float)-1.5994620704162712580875368683052 = -1.599462

	// absolute error = 0,00512
	// relative error = 0,3199%
}

float pow2(float i)
{
	const float PowBodge=0.33971f;
	float x;
	float y=i-(int)i;
	y=((y<0?-y:y)-y*y)*PowBodge;

	x=i+127-y;
	x*= shift23; //pow(2,23);
	//*(int*)&x=(int)x;	// Compiler doesn't compile this reliably into the below
	asm("trunc.w.s %0, %1\n"
		:"=f"(x):"f"(x));
	return x;
}

float powf( float a, float b )
{
	if (a <= 0.0001f) return 0.0f;
	return pow2(b*log2(a));
}

/*
float vpowf(float x, float y) {
	float result;
	__asm__ volatile (
		"mfv      $8, s000\n"
		"mfv      $9, s001\n"
		"mtv      %1, S000\n"
		"mtv      %2, S001\n"
		"vlog2.s  S001, S001\n"
		"vmul.s   S000, S000, S001\n"
		"vexp2.s  S000, S000\n"
		"mfv      %0, S000\n"
		"mtv      $8, s000\n"
		"mtv      $9, s001\n"
	: "=r"(result) : "r"(x), "r"(y) : "$8", "$9");
	return result;
}



float vatanf(float x)
{
	float result;
	__asm__ volatile (
		"mfv      $8, s000\n"
		"mfv      $9, s001\n"
		"mtv      %1, S000\n"
		"vmul.s   S001, S000, S000\n"
		"vadd.s   S001, S001, S001[1]\n"
		"vrsq.s   S001, S001\n"
		"vmul.s   S000, S000, S001\n"
		"vasin.s  S000, S000\n"
		"vcst.s   S001, VFPU_PI_2\n"
		"vmul.s   S000, S000, S001\n"
		"mfv      %0, S000\n"
		"mtv      $8, s000\n"
		"mtv      $9, s001\n"
	: "=r"(result) : "r"(x) : "$8","$9");
	return result;
}
*/

#define PI   3.14159265358979f
#define PI_2 1.57079632679489f

inline float fabsf(float x)
{
	float r;
	__asm__ volatile( "abs.s %0, %1" : "=f"(r) :"f"(x):"memory");
	return r;
}

/*
float vatan2f(float y, float x)
{
	if (x > 0.0f)
		return vatanf(y/x);
	if (x==0.0f)
		return (y < 0.0f ? -PI_2 : (y > 0.0f ? PI_2 : 0.0f));
	return (y < 0.0f ? vatanf(y/x) - PI : vatanf(y/x) + PI);
}
*/


// |error| < 0.005
float atan2f( float y, float x )
{
	if ( x == 0.0f )
	{
		if ( y > 0.0f ) return PI_2;
		if ( y == 0.0f ) return 0.0f;
		return -PI_2;
	}
	float atan;
	float z = y/x;
	if ( fabsf( z ) < 1.0f )
	{
		atan = z/(1.0f + 0.28f*z*z);
		if ( x < 0.0f )
		{
			if ( y < 0.0f ) return atan - PI;
			return atan + PI;
		}
	}
	else
	{
		atan = PI_2 - z/(z*z + 0.28f);
		if ( y < 0.0f ) return atan - PI;
	}
	return atan;
}
