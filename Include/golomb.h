#pragma once
#include "bitstream.h"
#include <memory>

namespace VideoCoding
{
	using std::vector; using std::string;
	class Golomb
	{
		public:
			Golomb(string fname, char mode);
			~Golomb();
			
			void encode(int value, int m_value);
			uint bytesEncoded();
			bool decode(int *ret, int m_value);
			void writeHeader(uint64_t value, uint m_value=0);
			bool readHeader(uint *ret, uint m_value=0);
			bool readHeader(uint64_t *ret, uint m);
			bool eof();
			void close();
			uint getUV(int value);
			int getSV(uint value);
		private:
			std::unique_ptr<BitStream::BitStream> bitstream;
			uint m;
			uint b;
			uint t;
			char mode;
	};
}
