#ifndef SF2_HPP
#define SF2_HPP

#include <stdio.h>
#include <stdint.h>
#include "sf2_types.hpp"

class InfoListChunk;
class SdtaListChunk;
class HydraChunk;
class sfPresetHeader;
class sfBag;
class sfInst;

// This is our own type, not part of the specification
typedef enum
{
	UNSIGNED_8,
	SIGNED_8,
	SIGNED_16,
	GAMEBOY_CH3,
	BDPCM
}
SampleType;

class SF2
{
	uint32_t size;
// Instrument and samples objects
//	SF2Instr& instruments;
//	gba_samples& samples;

	//All 3 chunks of the SF2 file
	InfoListChunk *const infolist_chunk;
	SdtaListChunk *const sdtalist_chunk;
	HydraChunk *const pdtalist_chunk;

	void add_terminals();

// Forbid copy and affectation
	SF2(SF2&);
	SF2& operator=(SF2&);

public:
	//Target file, should be assigned to a valid opened FILE in "wb" mode by user before "write()" is called.
	FILE *out;
	unsigned int default_sample_rate;

	SF2(unsigned int sample_rate);
	~SF2();
	void write(FILE *outfile);
	void add_new_preset(const char *name, int Patch, int Bank);
	void add_new_instrument(const char *name);
	void add_new_inst_bag();
	void add_new_preset_bag();
	void add_new_preset_modulator();
	void add_new_preset_generator();
	void add_new_preset_generator(SFGenerator operation, uint16_t value);
	void add_new_preset_generator(SFGenerator operation, uint8_t lo, uint8_t hi);
	void add_new_inst_modulator();
	void add_new_inst_generator();
	void add_new_inst_generator(SFGenerator operation, uint16_t value);
	void add_new_inst_generator(SFGenerator operation, uint8_t lo, uint8_t hi);
	void add_new_sample_header(const char *name, int start, int end, int start_loop, int end_loop, int sample_rate, int original_pitch, int pitch_correction);

	void add_new_sample(FILE *file, SampleType type, const char *name, uint32_t pointer, uint32_t size, bool loop_flag,
				  uint32_t loop_pos, uint32_t original_pitch, uint32_t pitch_correction, uint32_t sample_rate);
	// Add new sample using default sample rate
	inline void add_new_sample(FILE *file, SampleType type, const char *name, uint32_t pointer, uint32_t size,
					  bool loop_flag, uint32_t loop_pos, uint32_t original_pitch, uint32_t pitch_correction)
	{
		add_new_sample(file, type, name, pointer, size, loop_flag, loop_pos, original_pitch, pitch_correction, default_sample_rate);
	}

	uint16_t get_ibag_size();
	uint16_t get_igen_size();
	uint16_t get_imod_size();
	uint16_t get_pbag_size();
	uint16_t get_pgen_size();
	uint16_t get_pmod_size();
};

#endif