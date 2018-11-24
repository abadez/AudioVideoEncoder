#pragma once
#include <unistd.h>
#include <iostream>
#include <exception>
#include <fstream>


#include "file.h"
#include "bits.h"
#include "exception.h"

namespace BitStream
{
	using std::fstream;
	class BitStream
	{
		public:
			BitStream(string fname, char mode);
			~BitStream();
			
			bool writeByte(bits byte);
			bits readByte(bool reset=false);
			void writeBit(uint bit);
			void writeNBits(uint64_t bits, uint n_bits);
			uint readBit();
			bool readNBits(uint n_bits, uint *ret);
			bool readNBits(uint n_bits, uint64_t *ret);
			uint getTotal();
			bool eof();
			void close();
		private:
			bool flag = false;
			uint pos = 0;
			uint bytes = 0;
			fstream fs;
			bits b;
			char mode;
	};
}

