/*
 * Sound Font Instruments class
 * 
 * This program is part of GBA SoundFontRiper (c) 2012, 2014 by Bregalad
 * This is free and open source software.
 */

#ifndef GBA_INST_HPP
#define GBA_INST_HPP

#include <stdint.h>
#include <map>
#include "sf2.hpp"
#include "gba_samples.hpp"

struct inst_data
{
	uint32_t word0;
	uint32_t word1;
	uint32_t word2;
};

typedef std::map<inst_data, int>::iterator inst_it;

class GBAInstr
{
	int cur_inst_index;
	std::map<inst_data, int> inst_map;	// Contains pointers to instruments within GBA file, their position is the # of instrument in the SF2
	SF2 *sf2;										// Related .sf2 file
	GBASamples samples;								// Related samples class
	
	// Convert pointer from GBA memory map to ROM memory map
	uint32_t get_GBA_pointer();
	// Apply ADSR envelope on the instrument
	void generate_adsr_generators(const uint32_t adsr);
	void generate_psg_adsr_generators(const uint32_t adsr);

public:
	GBAInstr(SF2 *sf2) : cur_inst_index(0), sf2(sf2), samples(sf2)
	{}

	//Build a SF2 instrument form a GBA sampled instrument
	int build_sampled_instrument(const inst_data inst);

	//Create new SF2 from every key split GBA instrument
	int build_every_keysplit_instrument(const inst_data inst);

	//Build a SF2 instrument from a GBA key split instrument
	int build_keysplit_instrument(const inst_data inst);

	//Build gameboy channel 3 instrument
	int build_GB3_instrument(const inst_data inst);

	//Build GameBoy pulse wave instrument
	int build_pulse_instrument(const inst_data inst);

	//Build GameBoy white noise instrument
	int build_noise_instrument(const inst_data inst);
};
#endif