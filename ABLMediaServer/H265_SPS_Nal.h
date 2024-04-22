#ifndef _H265_SPS_H
#define _H265_SPS_H

#include "stdafx.h"

typedef unsigned char   uint8;
typedef unsigned short uint16;
typedef unsigned longuint32;
typedef unsigned __int64uint64;
typedef signed charint8;
typedef signed shortint16;
typedef signed longint32;
typedef signed __int64int64;

struct vc_params_t
{
	long          width, height;
	unsigned long profile, level;
	unsigned long nal_length_size;
	void clear()
	{
		memset(this, 0, sizeof(*this));
	}
};

class NALBitstream
{
public:
	NALBitstream() : m_data(NULL), m_len(0), m_idx(0), m_bits(0), m_byte(0), m_zeros(0)
	{

	};
	NALBitstream(void * data, int len)
	{
		Init(data, len);
	};
	void Init(void * data, int len)
	{
		m_data = (unsigned char*)data;
		m_len = len;
		m_idx = 0;
		m_bits = 0;
		m_byte = 0;
		m_zeros = 0;
	};
	unsigned char GetBYTE()
	{
		if (m_idx >= m_len)
			return 0;
		unsigned char b = m_data[m_idx++];
		if (b == 0)
		{
			m_zeros++;
			if ((m_idx < m_len) && (m_zeros == 2) && (m_data[m_idx] == 0x03))
			{
				m_idx++;
				m_zeros = 0;
			}
		}
		else  m_zeros = 0;

		return b;
	};

	unsigned int GetBit()
	{
		if (m_bits == 0)
		{
			m_byte = GetBYTE();
			m_bits = 8;
		}
		m_bits--;
		return (m_byte >> m_bits) & 0x1;
	};

	unsigned int GetWord(int bits)
	{
		unsigned int u = 0;
		while (bits > 0)
		{
			u <<= 1;
			u |= GetBit();
			bits--;
		}
		return u;
	};

	unsigned int GetUE()
	{
		int zeros = 0;
		while (m_idx < m_len && GetBit() == 0) zeros++;
		return GetWord(zeros) + ((1 << zeros) - 1);
	};

	unsigned int GetSE()
	{
		unsigned int UE = GetUE();
		bool positive = UE & 1;
		int  SE = (UE + 1) >> 1;
		if (!positive)
		{
			SE = -SE;
		}
		return SE;
	};
private:
	unsigned char*  m_data;
	int m_len;
	int m_idx;
	int m_bits;
	unsigned char m_byte;
	int m_zeros;
};

#endif