void ftoa(float fNum, char *str, int numDecPlaces)
{
    char sign = signbitf(fNum) ? '-' : '+';
    fNum = fabsf(fNum);

    str[0] = sign;
    ++str;
    
    float pow10 = 1.0f;
    int nLeadingDigits = int(truncf(log10f(fNum))) + 1;
    for (int i = 0; i < nLeadingDigits; ++i)
    {
		    int digit = long(truncf(fNum / pow10)) % 10;
		    pow10*= 10.0f;
	
        str[nLeadingDigits - 1 - i] = '0' + digit;
    }

    str[nLeadingDigits] = '.';
    
    pow10 = 1.0f;
    for (int decplace = 1; decplace <= numDecPlaces; ++decplace)
    {
        pow10*= 10.0f;
        int digit = long(trunc(fNum * pow10)) % 10;
        str[nLeadingDigits + decplace] = '0' + digit;
    }
    
    str[nLeadingDigits + numDecPlaces + 1] = 'f';
    str[nLeadingDigits + numDecPlaces + 2] = '\0';
}

void print_float(float num, int numDecPlaces) {
    char str[80] = "";
    ftoa(num, str, numDecPlaces);
    Serial.print(str);
}

bool isEOL(char ch)
{
    return ch >= 0x0A && ch <= 0x0D || ch == '\0';
}