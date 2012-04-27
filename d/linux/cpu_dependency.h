#pragma once

// i386
inline uint8_t test_bit(unsigned long bit, uint8_t array[])
{
	return array[bit/8] & (1<<(bit%8));
}
