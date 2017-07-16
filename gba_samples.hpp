/*
 * Sound Font Samples class
 * 
 * This program is part of GBA SoundFontRiper (c) 2012 by Bregalad
 * This is free and open source software.
 */

#ifndef GBA_SAMPLES_HPP
#define GBA_SAMPLES_HPP

#include "sf2.hpp"
#include <vector>

class GBASamples
{	// List of pointers to samples within the .gba file, position is # of sample in .sf2
	std::vector<uint32_t> samples_list;
	// Related sf2 class
	SF2 *sf2;

public:
	GBASamples(SF2 *sf2) : sf2(sf2)
	{}

	// Convert a normal sample to SoundFont format
	int build_sample(uint32_t pointer);
	// Convert a Game Boy channel 3 sample to SoundFont format
	int build_GB3_samples(uint32_t pointer);
	// Convert a Game Boy pulse (channels 1, 2) sample
	int build_pulse_samples(unsigned int duty_cycle);
	// Convert a Game Boy noise (channel 4) sample
	int build_noise_sample(bool metallic, int key);
};

#endif