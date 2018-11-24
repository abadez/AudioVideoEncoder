#include <cmath>
#include "golomb.h"

namespace VideoCoding
{
	Golomb::Golomb(string fname, char mode)
	{
		this->mode = mode;
		
		switch(mode)
		{
			case 'e':
						bitstream.reset(new BitStream::BitStream(fname, 'w'));
						break;
			
			case 'd':
						bitstream.reset(new BitStream::BitStream(fname, 'r'));
						break;
			default:	
						WrongModeException(mode);
						break;
		}
	}

	Golomb::~Golomb()
	{
		close();
	}

	void Golomb::encode(int value, int m_value)
	{
		uint positive_value = getUV(value);
		if(m == 0 || m != (uint)(abs(m_value) << 1))
		{
			if(m_value == 0) m_value = 1;
			m = abs(m_value) << 1;
			// calculate b
			b = (int) ceil( log2(m) );
			// calculate t
			t = (1 << b) - m;
		}
		
		uint quocient = positive_value / m;
		uint rest = positive_value % m;

		for (uint c = 0 ; c < quocient ; c++)
			bitstream->writeBit(1);
		bitstream->writeBit(0);

		if (rest < t)
			bitstream->writeNBits(rest, b-1);
		else
			bitstream->writeNBits(rest+t, b);
	}
	
	uint Golomb::bytesEncoded()
	{
		return bitstream->getTotal();
	}

	bool Golomb::decode(int *ret, int m_value)
	{
		uint rest = 0;
		if(m == 0 || m != (uint)(abs(m_value) << 1))
		{
			if(m_value == 0) m_value = 1;
			m = abs(m_value) << 1;
			// calculate b
			b = (int) ceil( log2(m) );
			// calculate t
			t = (1 << b) - m;
		}
		*ret = 0;
		while(!bitstream->eof() && bitstream->readBit() == 1)
			*ret += 1;
		if(bitstream->eof()) return false;

		uint r;
		if(!bitstream->readNBits(b-1, &r)) return false;

		if (r < t)
			rest = r;
		else
		{
			if(bitstream->eof()) return false;
			r = (r << 1) | bitstream->readBit();
			rest = r - t;
		}
		uint pos_result = *ret * m + rest;
		
		*ret = getSV(pos_result);
		return true;
	}
	
	void Golomb::writeHeader(uint64_t value, uint m)
	{
		bitstream->writeNBits(value, m);
	}

	bool Golomb::readHeader(uint *ret, uint m)
	{
		if(!bitstream->readNBits(m, ret)) return false;
		return true;
	}
	
	bool Golomb::readHeader(uint64_t *ret, uint m)
	{
		if(!bitstream->readNBits(m, ret)) return false;
		return true;
	}
	
	bool Golomb::eof()
	{
		return bitstream->eof();
	}
	
	void Golomb::close()
	{
		bitstream->close();
	}
	
	uint Golomb::getUV(int value)
	{
		if ( value < 0 )
		{
			return (abs(value) << 1) - 1;
		} 
		else
			return value << 1;
	}
	
	int Golomb::getSV(uint value)
	{
		if (value % 2 == 0)
			return value >>= 1;
		else
			return value = -((value + 1) >> 1);
	}
}
