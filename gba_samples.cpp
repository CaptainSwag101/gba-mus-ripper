/*
 * This file is part of GBA Sound Riper
 * (c) 2012, 2014 Bregalad
 * This is free and open source software
 *
 * This class deals with internal representation of
 * GBA samples and converts them to SF2 samples
 */
 
#include "gba_samples.hpp"
#include <stdint.h>
#include <math.h>
#include "hex_string.hpp"
#include <stdio.h>

extern FILE *inGBA;
extern FILE *psg_data;
extern FILE *goldensun_synth;

int GBASamples::build_sample(uint32_t pointer)
{	// Do nothing if sample already exists
	for(int i=samples_list.size()-1; i >= 0; --i)
		if(samples_list[i] == pointer) return i;

	// Read sample data
	if(fseek(inGBA, pointer, SEEK_SET)) throw -1;

	struct
	{
		uint32_t loop;
		uint32_t pitch;
		uint32_t loop_pos;
		uint32_t len;
	}
	hdr;
	fread(&hdr, 4, 4, inGBA);

	//Now we should make sure the data is coherent, and reject
	//the samples if errors are suspected

	//Detect invalid samples
	bool loop_en;
    bool bdpcm_en = false;
	
	if(hdr.loop == 0x40000000)
		loop_en = true;
	else if (hdr.loop == 0x00000000)
		loop_en = false;
	else if (hdr.loop == 0x1)
	{
		bdpcm_en = true;    // Detect compressed samples
	    loop_en = false;
	}
	else
		throw -1;			// Invalid loop -> return error

	// Compute SF2 base note and fine tune from GBA pitch
	// GBA pitch is 1024 * Mid_C frequency
	double delta_note = 12 * log2(sf2->default_sample_rate * 1024.0 / hdr.pitch);
	double int_delta_note = round(delta_note);
	unsigned int pitch_correction = int((int_delta_note - delta_note) * 100);
	unsigned int original_pitch = 60 + (int)int_delta_note;

	// Detect Golden Sun samples
	if(goldensun_synth && hdr.len == 0 && hdr.loop_pos == 0)
	{
		if(fgetc(inGBA) != 0x80) throw -1;
		uint8_t type = fgetc(inGBA);
		switch(type)
		{
			case 0:		// Square wave
			{
				std::string name = "Square @0x" + hex(pointer);
				uint8_t duty_cycle = fgetc(inGBA);
				uint8_t change_speed = fgetc(inGBA);
				if(change_speed == 0)
				{	// Square wave with constant duty cycle
					unsigned int base_pointer = 128 + 64 * (duty_cycle >> 2);
					sf2->add_new_sample(goldensun_synth, UNSIGNED_8, name.c_str(), base_pointer, 64, true, 0, original_pitch, pitch_correction);
				}
				else
				{	// Sqaure wave with variable duty cycle, not exact, but sounds close enough
					sf2->add_new_sample(goldensun_synth, UNSIGNED_8, name.c_str(), 128, 8192, true, 0, original_pitch, pitch_correction);
				}
			}	break;

			case 1:		// Saw wave
			{
				std::string name = "Saw @0x" + hex(pointer);
				sf2->add_new_sample(goldensun_synth, UNSIGNED_8, name.c_str(), 0, 64, true, 0, original_pitch, pitch_correction);
			}	break;

			case 2:		// Triangle wave
			{
				std::string name = "Triangle @0x" + hex(pointer);
				sf2->add_new_sample(goldensun_synth, UNSIGNED_8, name.c_str(), 64, 64, true, 0, original_pitch, pitch_correction);
			}	break;

			default :
				throw -1;
		}
	}
	else
	{
		//Prevent samples which are way too long or too short
		if (hdr.len < 16 || hdr.len > 0x3FFFFF) throw -1;

		//Prevent samples with illegal loop point from happening
		if (hdr.loop_pos > hdr.len-8)
		{
			puts("Warning : Illegal loop point detected\n");
			hdr.loop_pos = 0;
		}

		// Create (poetic) instrument name
		std::string name = (bdpcm_en ? "BDPCM @0x" : "Sample @0x") + hex(pointer);

		// Add the sample to output
		sf2->add_new_sample(inGBA, bdpcm_en ? BDPCM : SIGNED_8, name.c_str(), pointer + 16, hdr.len, loop_en, hdr.loop_pos, original_pitch, pitch_correction);
	}
	samples_list.push_back(pointer);
	return samples_list.size() - 1;
}

//Build game boy channel 3 sample
int GBASamples::build_GB3_samples(uint32_t pointer)
{
	// Do nothing if sample already exists
	for(int i=samples_list.size()-1; i >= 0; --i)
		if(samples_list[i] == pointer) return i;

	std::string name = "GB3 @0x" + hex(pointer);

	sf2->add_new_sample(inGBA, GAMEBOY_CH3, (name + 'A').c_str(), pointer, 256, true, 0, 53, 24, 22050);
	sf2->add_new_sample(inGBA, GAMEBOY_CH3, (name + 'B').c_str(), pointer, 128, true, 0, 65, 24, 22050);
	sf2->add_new_sample(inGBA, GAMEBOY_CH3, (name + 'C').c_str(), pointer, 64, true, 0, 77, 24, 22050);
	sf2->add_new_sample(inGBA, GAMEBOY_CH3, (name + 'D').c_str(), pointer, 32, true, 0, 89, 24, 22050);

	// We have to to add multiple entries to have the size of the list in sync
	// with the numeric indexes of samples....
	for(int i=4; i!=0; --i) samples_list.push_back(pointer);
	return samples_list.size() - 1;
}

//Build square wave sample
int GBASamples::build_pulse_samples(unsigned int duty_cycle)
{	// Do nothing if sample already exists
	for(int i=samples_list.size()-1; i >= 0; --i)
		if(samples_list[i] == duty_cycle) return i;

	std::string name = "square ";
	switch(duty_cycle)
	{
		case 0:
			name += "12.5%";
			break;
		
		default:
			name += "25%";
			break;

		case 2:
			name += "50%";
			break;
	}

	//This data is referenced to my set of recordings
	//stored in "psg_data.raw"
	const int pointer_tbl[3][5] =
	{
			{0x0000, 0x2166, 0x3c88, 0x4bd2, 0x698a},
			{0x7798, 0x903e, 0xa15e, 0xb12c, 0xbf4a},
			{0xc958, 0xe200, 0xf4ec, 0x10534, 0x11360}
	};

	const int size_tbl[3][5] =
	{
			{0x10b3, 0xd91, 0x7a5, 0xdec, 0x707},
			{0xc53, 0x890, 0x7e7, 0x70f, 0x507},
			{0xc54, 0x976, 0x824, 0x716, 0x36b}
	};

	const int loop_size[5] = {689, 344, 172, 86, 43};

	for(int i = 0; i < 5; i++)
	{
		sf2->add_new_sample(psg_data, SIGNED_16, (name + char('A' + i)).c_str(), pointer_tbl[duty_cycle][i], size_tbl[duty_cycle][i],
						  true, size_tbl[duty_cycle][i]-loop_size[i], 36 + 12 * i, 38, 44100);
		samples_list.push_back(duty_cycle);
	}
	return samples_list.size()-1;
}

//Build white noise sample
int GBASamples::build_noise_sample(bool metallic, int key)
{
	//prevent out of range keys
	if(key < 42) key = 42;
	if(key > 77) key = 76;

	unsigned int num = metallic ? 3 + (key-42) : 80 + (key-42);

	// Do nothing if sample already exists
	for(int i=samples_list.size()-1; i >= 0; --i)
		if(samples_list[i] == num) return i;

	std::string name = std::string("Noise ") + std::string(metallic ? "metallic " : "normal ") + std::to_string(key);

	const int pointer_tbl[] =
	{ 
		72246, 160446, 248646, 336846, 425046, 513246, 601446, 689646, 777846,
		866046, 954246, 1042446, 1130646, 1218846, 1307046, 1395246, 1483446, 1571646, 1659846,
		1748046, 1836246, 1924446, 2012646, 2100846, 2189046, 2277246, 2387493, 2475690, 2552863,
		2619011, 2674134, 2718233, 2756819, 2789893, 2817455, 2839504, 2856041, 2867066, 2872578
	};

	const int normal_len_tbl[] =
	{
		88200, 88200, 88200, 88200, 88200, 88200, 88200, 88200, 88200, 88200,
		88200, 88200, 88200, 88200, 88200, 88200, 88200, 88200, 88200, 88200, 88200, 88200, 88200,
		88200, 88200, 110247, 88197, 77173, 66148, 55123, 44099, 38586, 33074, 27562, 22049, 16537,
		11025, 5512, 2756
	};

	const int metallic_len_tbl[] =
	{	43755, 38286, 32817, 27347, 21878, 19143, 16408, 13674, 10939, 9572,
		8204, 6837, 5469, 4786, 4102, 3418, 2735, 2393, 2051, 1709, 1367, 1196, 1026, 855, 684,
		598, 513,	427, 342, 299, 256, 214, 171, 150, 128, 107, 85, 64
	};

	sf2->add_new_sample(psg_data, UNSIGNED_8, name.c_str(), pointer_tbl[key-42],
					  metallic ? metallic_len_tbl[key-42] : normal_len_tbl[key-42], true, 0, key, 0, 44100);

	samples_list.push_back(num);
	return samples_list.size() - 1;
}