
float atof( char* s )
{
	float sign = 1.0f;
	float value = 0.f;
	float mag = 10.f;
	int dec = 0;
	switch (*s)
	{
		case '-': sign = -1.f;
		case '+': s++;
	}
	
	int exp = 0;
	while ((*s>='0' && *s<='9') || (!dec && *s=='.'))
	{
		if (*s=='.')
			dec = 1;
		else
		{
			value = value * 10.f + (float)(*s - '0');
			if (dec)
				exp--;
		}
		s++;
	}
	if (sign<0)
		value = -value;
	
	// Handle exponents
	if (*s=='e'||*s=='E')
	{
		int esign = 1;
		switch (*++s)
		{
			case '-': esign = -1;
			case '+': s++;
		}

		int val = 0;
		while (*s>='0' && *s<='9')
		{
			val = val*10 + (*s - '0');
			s++;
		}
		if (esign==1)
			exp += val;
		else
			exp -= val;
	}
	
	int n = (exp>=0?exp:-exp);
	while (n>0)
	{
		if (n&1)
		{
			if (exp>0)
				value *= (float)mag;
			else
				value /= (float)mag;
		}
		mag *= mag;
		n >>= 1;
	}
	
	return value;
}


int atoi( char* s )
{
	int sign = 1;
	int value = 0;
	if (*s=='-')
	{
		sign = -1;
		s++;
	}
	else
	if (*s=='+')
		s++;

	while (*s>='0' && *s<='9')
	{
		value = value * 10 + (*s - '0');
		s++;
	}
	return sign*value;
}


int	strncmpupr( char* s1, char* s2, int n )
{
	while (n && *s1 && *s2)
	{
		char c1 = *s1++;
		char c2 = *s2++;

		if ((c1 >= 'a') && (c1 <= 'z'))
			c1 -= 'a' - 'A';
		if ((c2 >= 'a') && (c2 <= 'z'))
			c2 -= 'a' - 'A';

		if (c1 > c2)
			return 1;
		if (c1 < c2)
			return -1;
		n--;
	}
	
	if (n && *s1)
		return 1;
	if (n && *s2)
		return -1;

	return 0;
}
