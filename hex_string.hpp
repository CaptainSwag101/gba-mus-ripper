/*
 * This file is part of GBA Sound Riper
 * (c) 2012, 2014 Bregalad
 * This is free and open source software
 *
 * This file provides a simple quick hex -> std::string conversion function
 */

#ifndef HEX_STRING_HPP
#define HEX_STRING_HPP
#include <string>
#include <stdint.h>

static std::string hex(const uint32_t n)
{
	std::string s;
	for(int i=7; i >= 0; --i)
	{
		s += "0123456789abcdef"[0xf & (n >> (4*i))];
	}
	// Remove leading zeroes
	return s.substr(s.find_first_not_of('0'));
}
#endif