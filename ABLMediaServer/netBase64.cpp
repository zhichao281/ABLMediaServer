#include "stdafx.h"
#include "netBase64.h"  
  
static const char
_B64_[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
'w', 'x', 'y', 'z', '0', '1', '2', '3',
'4', '5', '6', '7', '8', '9', '+', '/'
};

//--------------------------------------------------
//Encodeing:
//--------------------------------------------------
 void encodetribyte(unsigned char * in, unsigned char * out, int len)
{
	if (len == 0) return;
	int i;
	unsigned char inbuf[3];
	memset(inbuf, 0, sizeof(unsigned char) * 3);
	for (i = 0; i<len; i++)
	{
		inbuf[i] = in[i];
	}
	out[0] = _B64_[inbuf[0] >> 2];
	out[1] = _B64_[((inbuf[0] & 0x03) << 4) | ((inbuf[1] & 0xf0) >> 4)];
	//if(len>1) means len=={2,3}
	//else means len==1, out[2]='=';
	out[2] = (len>1 ? _B64_[((inbuf[1] & 0x0f) << 2) | ((inbuf[2] & 0xc0) >> 6)] : '=');
	//if(len>2) menas len==3
	//else means len=={1,2} out[3]='=';
	out[3] = (len>2 ? _B64_[inbuf[2] & 0x3f] : '=');
}

//--------------------------------------------------
//Decoding:
//--------------------------------------------------
int decodetribyte(unsigned char * in, unsigned char * out)
{
	int i, j, len;
	char dec[4];
	memset(dec, 0, sizeof(char) * 4);
	len = 3;
	//Get effective original text char count:
	if (in[3] == '=') len--;
	if (in[2] == '=') len--;
	//Find code according to input charactors:
	for (i = 0; i<64; i++)
	{
		for (j = 0; j<4; j++)
		{
			if (in[j] == _B64_[i]) dec[j] = i;
		}
	}
	//Re-compose original text code:
	out[0] = (dec[0] << 2 | dec[1] >> 4);
	if (len == 1) return 1;
	out[1] = (dec[1] << 4 | dec[2] >> 2);
	if (len == 2) return 2;
	out[2] = (((dec[2] << 6) & 0xc0) | dec[3]);
	return 3;
}

//Encode input byte stream, please ensure lenghth of out
//buffer big enough to hold all codes.
//The b64 code array size is 4*(tri-byte blocks in original text).
int Base64Encode(unsigned char * b64, const unsigned char * input, unsigned long stringlen )
{
	if (!b64 || !input || stringlen<0) return 0;
	unsigned long slen, imax;
	register unsigned int i, idin, idout;
	int rd, re, len;
	slen = (stringlen) ? stringlen : strlen((char *)input);
	if (slen == 0) return 0;
	rd = slen % 3;
	rd = (rd == 0) ? 3 : rd;
	//Maximun tri-byte blocks:
	imax = (slen + (3 - rd)) / 3 - 1;
	for (i = 0; i <= imax; i++)
	{
		idin = i * 3;
		idout = i * 4;
		len = (i == imax) ? rd : 3;
		encodetribyte((unsigned char *)&input[idin], &b64[idout], len);
	}
	re = (imax + 1) * 4;
	b64[re] = '\0';
	return re;
}

//Decode input byte stream, please ensure lenghth of out
//buffer big enough to hold all codes.
//The b64 code array size is about 3*(quad-byte blocks in b64 code).
int Base64Decode(unsigned char * output, const unsigned char * b64, unsigned long codelen )
{
	if (!output || !b64 || codelen<0) return 0;
	unsigned long slen, imax;
	register unsigned int i, idin, idout;
	int rd, re, len;
	slen = (codelen) ? codelen : strlen((char *)b64);
	if (slen<4) return 0;
	rd = slen % 4;
	if (rd != 0) return 0;    //Code error!.
	imax = slen / 4 - 1;
	for (i = 0; i <= imax; i++)
	{
		idin = i * 4;
		idout = i * 3;
		len = decodetribyte((unsigned char *)&b64[idin], &output[idout]);
	}
	re = (imax * 3) + len;
	output[re] = '\0';
	return re;
}

