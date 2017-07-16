/*
 * This file is part of GBA Sound Riper
 * (c) 2012, 2014 Bregalad
 * This is free and open source software
 *
 * This class deals with internal representation of
 * GBA instruments and converts them to SF2 instruments
 */
#include "gba_instr.hpp"
#include <math.h>
#include "hex_string.hpp"
#include <vector>
#include <stdio.h>
extern FILE *inGBA;					// Related .gba file

bool operator <(const inst_data&i, const inst_data& j)
{
	if(j.word2 != i.word2) return i.word2 < j.word2;
	else if(j.word1 != i.word1) return i.word1 < j.word1;
	else return i.word0 < j.word0;
}

uint32_t GBAInstr::get_GBA_pointer()
{
	uint32_t p;
	fread(&p, 4, 1, inGBA);
	return p & 0x3FFFFFF;
}

void GBAInstr::generate_adsr_generators(const uint32_t adsr)
{
	// Get separate components
	int attack = adsr & 0xFF;
	int decay = (adsr>>8) & 0xFF;
	int sustain = (adsr>>16) & 0xFF;
	int release = adsr>>24;

	// Add generators for ADSR envelope if required
	if(attack != 0xFF)
	{
		// Compute attack time - the sound engine is called 60 times per second
		// and adds "attack" to envelope every time the engine is called
		double att_time = (256/60.0) / attack;
		double att = 1200 * log2(att_time);
		sf2->add_new_inst_generator(SFGenerator::attackVolEnv, uint16_t(att));
	}

	if(sustain != 0xFF)
	{
		double sus;
		// Compute attenuation in cB if sustain is non-zero
		if(sustain != 0) sus = 100 * log(256.0/sustain);
		// Special case where attenuation is infinite -> use max value
		else sus = 1000;

		sf2->add_new_inst_generator(SFGenerator::sustainVolEnv, uint16_t(sus));

		double dec_time = (log(256.0) /(log(256)-log(decay)))/60.0;
		dec_time *= 10/log(256);
		double dec = 1200 * log2(dec_time);
		sf2->add_new_inst_generator(SFGenerator::decayVolEnv, uint16_t(dec));
	}

	if(release != 0x00)
	{
		double rel_time = (log(256.0)/(log(256)-log(release)))/60.0;
		double rel = 1200 * log2(rel_time);
		sf2->add_new_inst_generator(SFGenerator::releaseVolEnv, uint16_t(rel));
	}
}

void GBAInstr::generate_psg_adsr_generators(const uint32_t adsr)
{
	// Get separate components
	int attack = adsr & 0xFF;
	int decay = (adsr>>8) & 0xFF;
	int sustain = (adsr>>16) & 0xFF;
	int release = adsr>>24;

	// Reject instrument if invalid values !
	if(attack > 15 || decay > 15 || sustain > 15 || release > 15) throw -1;

	// Add generators for ADSR envelope if required
	if(attack != 0)
	{
		// Compute attack time - the sound engine is called 60 times per second
		// and adds "attack" to envelope every time the engine is called
		double att_time = attack/5.0;
		double att = 1200 * log2(att_time);
		sf2->add_new_inst_generator(SFGenerator::attackVolEnv, uint16_t(att));
	}

	if(sustain != 15)
	{
		double sus;
		// Compute attenuation in cB if sustain is non-zero
		if(sustain != 0) sus = 100 * log(15.0/sustain);
		// Special case where attenuation is infinite -> use max value
		else sus = 1000;

		sf2->add_new_inst_generator(SFGenerator::sustainVolEnv, uint16_t(sus));

		double dec_time = decay/5.0;
		double dec = 1200 * log2(dec_time+1);
		sf2->add_new_inst_generator(SFGenerator::decayVolEnv, uint16_t(dec));
	}

	if(release != 0)
	{
		double rel_time = release/5.0;
		double rel = 1200 * log2(rel_time);
		sf2->add_new_inst_generator(SFGenerator::releaseVolEnv, uint16_t(rel));
	}
}

// Build a SF2 instrument form a GBA sampled instrument
int GBAInstr::build_sampled_instrument(const inst_data inst)
{
	// Do nothing if this instrument already exists !
	inst_it it = inst_map.find(inst);
	if(it != inst_map.end()) return (*it).second;

	// The flag is set if no scaling should be done if the instrument type is 8
	bool no_scale = (inst.word0&0xff) == 0x08;

	// Get sample pointer
	uint32_t sample_pointer = inst.word1 & 0x3ffffff;

	// Determine if loop is enabled (it's dumb but we have to seek just for this)
	if(fseek(inGBA, sample_pointer|3, SEEK_SET)) throw -1;
	bool loop_flag = fgetc(inGBA) == 0x40;

	// Build pointed sample
	int sample_index = samples.build_sample(sample_pointer);

	// Instrument's name
	std::string name = "sample @0x" + hex(sample_pointer);

	// Create instrument bag
	sf2->add_new_instrument(name.c_str());
	sf2->add_new_inst_bag();

	// Add generator to prevent scaling if required
	if(no_scale)
		sf2->add_new_inst_generator(SFGenerator::scaleTuning, 0);

	generate_adsr_generators(inst.word2);
	sf2->add_new_inst_generator(SFGenerator::sampleModes, loop_flag ? 1 : 0);
	sf2->add_new_inst_generator(SFGenerator::sampleID, sample_index);

	// Add instrument to list
	inst_map[inst] = cur_inst_index;
	return cur_inst_index ++;
}

// Create new SF2 from every key split GBA instrument
int GBAInstr::build_every_keysplit_instrument(const inst_data inst)
{
	// Do nothing if this instrument already exists !
	inst_it it = inst_map.find(inst);
	if(it != inst_map.end()) return (*it).second;

	// I'm sorry for doing a dumb copy/pase of the routine right above
	// But there was too much differences to handles to practically handle it with flags
	// therefore I didn't really had a choice.
	uint32_t baseaddress = inst.word1 & 0x3ffffff;
	std::string name = "EveryKeySplit @0x" + hex(baseaddress);
	sf2->add_new_instrument(name.c_str());

	// Loop through all keys
	for(int key = 0; key < 128; key ++)
	{
		try
		{
			// Seek at the key's instrument
			if(fseek(inGBA, baseaddress + 12*key, SEEK_SET)) throw -1;

			// Read instrument data
			int instrType = fgetc(inGBA);		// Instrument type
			int keynum = fgetc(inGBA);			// Key (every key split instrument only)
		/*  int unused_byte =*/ fgetc(inGBA);		// Unknown/unused byte
			int panning = fgetc(inGBA);			// Panning (every key split instrument only)

			// The flag is set if no scaling should be done on the sample
			bool no_scale = false;

			uint32_t main_word, adsr;
			fread(&main_word, 4, 1, inGBA);

			// Get ADSR envelope
			fread(&adsr, 4, 1, inGBA);

			int sample_index;
			bool loop_flag = true;

			switch(instrType & 0x0f)
			{
				case 8:
					no_scale = true;
				case 0:
				{
					// Determine if loop is enabled and read sample's pitch
					uint32_t sample_pointer = main_word & 0x3ffffff;
					if(fseek(inGBA, sample_pointer|3, SEEK_SET)) throw -1;
					loop_flag = fgetc(inGBA) == 0x40;

					uint32_t pitch;
					fread(&pitch, 4, 1, inGBA);

					// Build pointed sample
					sample_index = samples.build_sample(sample_pointer);

					// Add a bag for this key
					sf2->add_new_inst_bag();
					generate_adsr_generators(adsr);
					// Add generator to prevent scaling if required
					if(no_scale)
						sf2->add_new_inst_generator(SFGenerator::scaleTuning, 0);

					// Compute base note and fine tune from pitch
					double delta_note = 12.0 * log2(sf2->default_sample_rate * 1024.0 / pitch);
					int rootkey = 60 + int(round(delta_note));

					// Override root key with the value we need
					sf2->add_new_inst_generator(SFGenerator::overridingRootKey, rootkey - keynum + key);
					// Key range is only a single key (obviously)
					sf2->add_new_inst_generator(SFGenerator::keyRange, key, key);
				}	break;

				case 4:
				case 12:
				{
					// Determine whenever the note is metallic noise, normal noise, or invalid
					bool metal_flag;
					if(main_word == 0x1000000)
						metal_flag = true;
					else if(main_word == 0)
						metal_flag = false;
					else
						throw -1;

					// Build corresponding sample
					sample_index = samples.build_noise_sample(metal_flag, keynum);
					sf2->add_new_inst_bag();
					generate_psg_adsr_generators(adsr);
					sf2->add_new_inst_generator(SFGenerator::overridingRootKey, key);
					sf2->add_new_inst_generator(SFGenerator::keyRange, key, key);
				}	break;

				// Ignore other kind of instruments
				default : throw -1;
			}

			if(panning != 0)
				sf2->add_new_inst_generator(SFGenerator::pan, int((panning-192) * (500/128.0)));
			// Same as a normal sample
			sf2->add_new_inst_generator(SFGenerator::sampleModes, loop_flag ? 1 : 0);
			sf2->add_new_inst_generator(SFGenerator::sampleID, sample_index);
		}
		catch(...) {}	// Continue to next key when there is a major problem
	}
	// Add instrument to list
	inst_map[inst] = cur_inst_index;
	return cur_inst_index ++;
}

// Build a SF2 instrument from a GBA key split instrument
int GBAInstr::build_keysplit_instrument(const inst_data inst)
{
	// Do nothing if this instrument already exists !
	inst_it it = inst_map.find(inst);
	if(it != inst_map.end()) return (*it).second;

	uint32_t base_pointer = inst.word1 & 0x3ffffff;
	uint32_t key_table = inst.word2 & 0x3ffffff;

	// Decode key-table in usable data
	std::vector<int8_t> split_list, index_list;

	int8_t key = 0;
	int prev_index = -1;
	int current_index;
	if(fseek(inGBA, key_table, SEEK_SET)) throw -1;

	// Add instrument to list
	std::string name = "0x" + hex(base_pointer) + " key split";
	sf2->add_new_instrument(name.c_str());

	do
	{
		int index = fgetc(inGBA);

		// Detect where there is changes in the index table
		current_index = index;
		if(prev_index != current_index)
		{
			split_list.push_back(key);
			index_list.push_back(current_index);
			prev_index = current_index;
		}
	}
	while(++key > 0);

	// Final entry for the last split
	split_list.push_back(0x80);

	for(unsigned int i=0; i<index_list.size(); i++)
	{
		try
		{
			// Seek to pointed instrument
			if(fseek(inGBA, base_pointer + 12*index_list[i], SEEK_SET)) throw -1;

			// Once again I'm sorry for the dumb copy/pase
			// but doing it all with flags would have been quite complex

			int inst_type = fgetc(inGBA);		// Instrument type
		 /* int keynum = */ fgetc(inGBA);		// Key (every key split instrument only)
		 /* int unused_byte = */ fgetc(inGBA);	// Unknown/unused byte
		 /* int panning = */ fgetc(inGBA);		// Panning (every key split instrument only)

			// The flag is set if no scaling should be done on the sample
			bool no_scale = inst_type==8;

			// Get sample pointer
			uint32_t sample_pointer;
			fread(&sample_pointer, 1, 4, inGBA);
			sample_pointer &= 0x3ffffff;

			// Get ADSR envelope
			uint32_t adsr;
			fread(&adsr, 4, 1, inGBA);

			// For now GameBoy instruments aren't supported
			// (I wonder if any game ever used this)
			if((inst_type & 0x07) != 0) continue;

			// Determine if loop is enabled (it's dumb but we have to seek just for this)
			if(fseek(inGBA, sample_pointer|3, SEEK_SET)) throw -1;
			bool loop_flag = fgetc(inGBA) == 0x40;

			// Build pointed sample
			int sample_index = samples.build_sample(sample_pointer);

			// Create instrument bag
			sf2->add_new_inst_bag();

			// Add generator to prevent scaling if required
			if(no_scale)
				sf2->add_new_inst_generator(SFGenerator::scaleTuning, 0);

			generate_adsr_generators(adsr);
			// Particularity here : An additional bag to select the key range
			sf2->add_new_inst_generator(SFGenerator::keyRange, split_list[i], split_list[i+1]-1);
			sf2->add_new_inst_generator(SFGenerator::sampleModes, loop_flag ? 1 : 0);
			sf2->add_new_inst_generator(SFGenerator::sampleID, sample_index);
		}
		catch(...) {}		// Silently continue to next key if anything bad happens
	}
	inst_map[inst] = cur_inst_index;
	return cur_inst_index ++;
}

// Build gameboy channel 3 instrument
int GBAInstr::build_GB3_instrument(const inst_data inst)
{
	// Do nothing if this instrument already exists !
	inst_it it = inst_map.find(inst);
	if(it != inst_map.end()) return (*it).second;

	// Get sample pointer
	uint32_t sample_pointer = inst.word1 & 0x3ffffff;

	// Try to seek to see if the pointer is valid, if it's not then abort
	if(fseek(inGBA, sample_pointer, SEEK_SET)) throw -1;

	int sample = samples.build_GB3_samples(sample_pointer);
	
	std::string name = "GB3 @0x" + hex(sample_pointer);
	sf2->add_new_instrument(name.c_str());

	// Global zone
	sf2->add_new_inst_bag();
	generate_psg_adsr_generators(inst.word2);

	sf2->add_new_inst_bag();
	sf2->add_new_inst_generator(SFGenerator::keyRange, 0, 52);
	sf2->add_new_inst_generator(SFGenerator::sampleModes, 1);
	sf2->add_new_inst_generator(SFGenerator::sampleID, sample-3);
	sf2->add_new_inst_bag();
	sf2->add_new_inst_generator(SFGenerator::keyRange, 53, 64);
	sf2->add_new_inst_generator(SFGenerator::sampleModes, 1);
	sf2->add_new_inst_generator(SFGenerator::sampleID, sample-2);
	sf2->add_new_inst_bag();
	sf2->add_new_inst_generator(SFGenerator::keyRange, 65, 76);
	sf2->add_new_inst_generator(SFGenerator::sampleModes, 1);
	sf2->add_new_inst_generator(SFGenerator::sampleID, sample-1);
	sf2->add_new_inst_bag();
	sf2->add_new_inst_generator(SFGenerator::keyRange, 77, 127);
	sf2->add_new_inst_generator(SFGenerator::sampleModes, 1);
	sf2->add_new_inst_generator(SFGenerator::sampleID, sample);

	inst_map[inst] = cur_inst_index;
	return cur_inst_index ++;
}

// Build GameBoy pulse wave instrument
int GBAInstr::build_pulse_instrument(const inst_data inst)
{
	// Do nothing if this instrument already exists !
	inst_it it = inst_map.find(inst);
	if(it != inst_map.end()) return (*it).second;

	unsigned int duty_cycle = inst.word1;
	// The difference between 75% and 25% duty cycles is inaudible therefore
	// I simply replace 75% duty cycles by 25%
	if(duty_cycle == 3) duty_cycle = 1;
	if(duty_cycle > 3) throw -1;

	int sample = samples.build_pulse_samples(duty_cycle);
	std::string name = "pulse " + std::to_string(duty_cycle);
	sf2->add_new_instrument(name.c_str());

	// Global zone
	sf2->add_new_inst_bag();
	generate_psg_adsr_generators(inst.word2);

	sf2->add_new_inst_bag();
	sf2->add_new_inst_generator(SFGenerator::keyRange, 0, 45);
	sf2->add_new_inst_generator(SFGenerator::sampleModes, 1);
	sf2->add_new_inst_generator(SFGenerator::sampleID, sample-4);
	sf2->add_new_inst_bag();
	sf2->add_new_inst_generator(SFGenerator::keyRange, 46, 57);
	sf2->add_new_inst_generator(SFGenerator::sampleModes, 1);
	sf2->add_new_inst_generator(SFGenerator::sampleID, sample-3);
	sf2->add_new_inst_bag();
	sf2->add_new_inst_generator(SFGenerator::keyRange, 58, 69);
	sf2->add_new_inst_generator(SFGenerator::sampleModes, 1);
	sf2->add_new_inst_generator(SFGenerator::sampleID, sample-2);
	sf2->add_new_inst_bag();
	sf2->add_new_inst_generator(SFGenerator::keyRange, 70, 81);
	sf2->add_new_inst_generator(SFGenerator::sampleModes, 1);
	sf2->add_new_inst_generator(SFGenerator::sampleID, sample-1);
	sf2->add_new_inst_bag();
	sf2->add_new_inst_generator(SFGenerator::keyRange, 82, 127);
	sf2->add_new_inst_generator(SFGenerator::sampleModes, 1);
	sf2->add_new_inst_generator(SFGenerator::sampleID, sample);

	inst_map[inst] = cur_inst_index;
	return cur_inst_index ++;
}

// Build GameBoy white noise instrument
int GBAInstr::build_noise_instrument(const inst_data inst)
{
	// Do nothing if this instrument already exists !
	inst_it it = inst_map.find(inst);
	if(it != inst_map.end()) return (*it).second;

	// 0 = normal, 1 = metallic, anything else = invalid
	if(inst.word1 > 1) throw -1;
	bool metallic = inst.word1;

	std::string name = metallic ? "GB metallic noise" : "GB noise";
	sf2->add_new_instrument(name.c_str());

	// Global zone
	sf2->add_new_inst_bag();
	generate_psg_adsr_generators(inst.word2);

	sf2->add_new_inst_bag();
	int sample42 = samples.build_noise_sample(metallic, 42);
	sf2->add_new_inst_generator(SFGenerator::keyRange, 0, 42);
	sf2->add_new_inst_generator(SFGenerator::sampleModes, 1);
	sf2->add_new_inst_generator(SFGenerator::sampleID, sample42);

	for(int key = 43; key <=77; key++)
	{
		sf2->add_new_inst_bag();
		int sample = samples.build_noise_sample(metallic, key);
		sf2->add_new_inst_generator(SFGenerator::keyRange, key, key);
		sf2->add_new_inst_generator(SFGenerator::sampleModes, 1);
		sf2->add_new_inst_generator(SFGenerator::sampleID, sample);		
	}

	sf2->add_new_inst_bag();
	int sample78 = samples.build_noise_sample(metallic, 78);
	sf2->add_new_inst_generator(SFGenerator::keyRange, 78, 127);
	sf2->add_new_inst_generator(SFGenerator::sampleModes, 1);
	sf2->add_new_inst_generator(SFGenerator::scaleTuning, 0);
	sf2->add_new_inst_generator(SFGenerator::sampleID, sample78);

	inst_map[inst] = cur_inst_index;
	return cur_inst_index ++;
}