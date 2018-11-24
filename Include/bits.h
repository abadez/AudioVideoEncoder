/* 
   Usage example:
   * bits myvar = {10}; // 00001010
   * printf("%d\n", myvar.getbit(1)); // prints 1
   * myvar.setbit(0, 1); // set bit 0 with value 1
   * printf("%d\n", myvar.value); // prints 11
*/
#ifndef __BITS_H_
#define __BITS_H_ 1
union bits
{
	char value;
	struct {
#if __BYTE_ORDER == __BIG_ENDIAN
		unsigned int _7:1; // [7] MSB
		unsigned int _6:1; // [6]
		unsigned int _5:1; // [5]
		unsigned int _4:1; // [4]
		unsigned int _3:1; // [3]
		unsigned int _2:1; // [2]
		unsigned int _1:1; // [1]
		unsigned int _0:1; // [0] LSB
#elif __BYTE_ORDER == __LITTLE_ENDIAN
		unsigned int _0:1; // [0] LSB
		unsigned int _1:1; // [1]
		unsigned int _2:1; // [2]
		unsigned int _3:1; // [3]
		unsigned int _4:1; // [4]
		unsigned int _5:1; // [5]
		unsigned int _6:1; // [6]
		unsigned int _7:1; // [7] MSB
#else
	#error "Cannot determine Endian of this machine"
#endif
	} __attribute__ ((__packed__)) sbits;
	unsigned int getbit(unsigned int n)
	{
		switch(n)
		{
			case 0: return sbits._0;
			case 1: return sbits._1;
			case 2: return sbits._2;
			case 3: return sbits._3;
			case 4: return sbits._4;
			case 5: return sbits._5;
			case 6: return sbits._6;
			case 7: return sbits._7;
			default: return 0;
		}
	}
	void setbit(unsigned int n, unsigned int val)
	{
		switch(n)
		{
			case 0: sbits._0 = val; break;
			case 1: sbits._1 = val; break;
			case 2: sbits._2 = val; break;
			case 3: sbits._3 = val; break;
			case 4: sbits._4 = val; break;
			case 5: sbits._5 = val; break;
			case 6: sbits._6 = val; break;
			case 7: sbits._7 = val; break;
			default: return;
		}
	}
};
typedef union bits bits;
#endif /* bits.h */
