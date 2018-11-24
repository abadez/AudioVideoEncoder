//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "bitstream.h"

namespace BitStream
{
	BitStream::BitStream(string fname, char mode)
	{
		this->mode = mode;
		switch(mode)
		{
			case 'w':
						fs.open(fname, fstream::out | fstream::binary);
						if(!fs.is_open())
							AccessDeniedException(fname);
						flag = true;
						break;
			
			case 'r':
						if(!file::fileExists(fname))
							FileNotFoundException(fname);
						fs.open(fname, fstream::in | fstream::binary);
						if(!fs.is_open())
							AccessDeniedException(fname);
						break;
			default:	
						WrongModeException(mode);
						break;
		}
	}
	
	BitStream::~BitStream()
	{
		fs.close();
	}
	
	bool BitStream::writeByte(bits byte)
	{
		if(!flag) WrongModeException(mode);
		if(pos > 0)
		{
			fs << b.value;
			b.value = 0;
			pos = 0;
			bytes++;
		}
		fs << byte.value;
		bytes++;
		return true;
	}
	
	bits BitStream::readByte(bool reset)
	{
		if(flag) WrongModeException(mode);
		if(pos == 0) b = { (char)fs.get() };
		else if(reset) pos = 0;
		return b;
	}
	
	void BitStream::writeBit(uint bit)
	{
		if(!flag) WrongModeException(mode);
		b.setbit(((++pos %= 8)+7)%8, bit);
		if(pos == 0)
		{
			fs << b.value;
			bytes++;
			b.value = 0;
		}
	}

	void BitStream::writeNBits(uint64_t val, uint n_bits)
	{
		if(!flag) WrongModeException(mode);
		
		for (int c = n_bits - 1 ; c >= 0 ; c--)
		{
			writeBit((val >> c) & 0b1);
		}	
	}
	
	uint BitStream::readBit()
	{
		if(flag) WrongModeException(mode);
		if(pos == 0) b = { (char)fs.get() };
		return b.getbit(((++pos %= 8)+7)%8);
	}

	bool BitStream::readNBits(uint n_bits, uint* ret)
	{
		if(flag) WrongModeException(mode);

		uint bits = 0;
		for (uint c = 0 ; c < n_bits ; c++)
		{
			if(eof()) return false;
			bits <<= 1;
			bits |= readBit();
		}

		*ret = bits;
		return true;
	}
	
	bool BitStream::readNBits(uint n_bits, uint64_t* ret)
	{
		if(flag) WrongModeException(mode);

		uint64_t bits = 0;
		for (uint c = 0 ; c < n_bits ; c++)
		{
			if(eof()) return false;
			bits <<= 1;
			bits |= readBit();
		}

		*ret = bits;
		return true;
	}
	
	uint BitStream::getTotal()
	{
		return bytes;
	}
	
	bool BitStream::eof()
	{
		if(flag) return true;
		if(pos == 0 && fs.peek() == EOF) return true;
		return false;
	}
	
	void BitStream::close()
	{
		if(pos > 0 && flag) // padding with 1's
		{
			while(++pos%9 != 0) b.setbit(pos-1, 1);
			fs << b.value;
			bytes++;
			pos = 0;
		}
		fs.close();
	}
}
