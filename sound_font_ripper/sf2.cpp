/**
 * Sound Font main class
 * 
 * This is part of the GBA SoundFont Ripper (c) 2012 by Bregalad
 * This is free and open source software.
 * 
 * Notes : I tried to separate the GBA-related stuff and SF2 related stuff as much as possible in different classes
 * that way anyone can re-use this program for building SF2s out of different data.
 * 
 * SF2 is a SoundFont file format. This class (and related classes) serves to create data to build
 * a SF2 file. For more details look at the doccument "Sound Font(R) Technical Doccumentation" version 2.01
 * which was used as a reference when writing these classes.
 * By the way I must tell this format is incredibly stupid and complex as opposed to what it could be and
 * I have no idea why people started to adopt this standard. But anyways they did...
 * 
 * All SF2-related classes contains a field named sf2 that links to the SF2 main class to know which
 * SF2 file they relates to. (this would make it possible to build multiple SF2 files at a time).
 * 
 * The building of a SF2 file is done in multiple passes :
 * - Creating samples
 * - Creating instruments
 * - Creating presets
 * - Output to the target file
 */

#include <stdio.h>
#include <string.h>
#include "sf2.hpp"
#include "sf2_chunks.hpp"
#include "gba_samples.hpp"

//Constructor to change the default sample rate
SF2::SF2(unsigned int sample_rate = 22050) :
	size(0), /*instruments(this),*/
	infolist_chunk(new InfoListChunk(this)),
	sdtalist_chunk(new SdtaListChunk(this)),
	pdtalist_chunk(new HydraChunk(this)),
	default_sample_rate(sample_rate)
{}

SF2::~SF2()
{
	delete infolist_chunk;
	delete sdtalist_chunk;
	delete pdtalist_chunk;
}

//Write data to the target file
//(should only be called once !)
void SF2::write(FILE *outfile)
{
	out = outfile;
	// This function adds the "terminal" data in subchunks that are required
	// by the (retarded) SF2 standard
	add_terminals();

	//Compute size of the entire file
	//(this will also compute the size of the chunks)
	size = 4;
	size += infolist_chunk->calcSize() + 8;
	size += sdtalist_chunk->calcSize() + 8;
	size += pdtalist_chunk->calcSize() + 8;

	//Write RIFF header
	fwrite("RIFF", 1, 4, out);
	fwrite(&size, 4, 1, out);
	fwrite("sfbk", 1, 4, out);

	//Write all 3 chunks
	infolist_chunk->write();
	sdtalist_chunk->write();
	pdtalist_chunk->write();

	//Close output file
	fclose(out);
}

//Add terminal data in subchunks where it is required by the standard
void SF2::add_terminals()
{
	add_new_sample_header("EOS", 0, 0, 0, 0, 0, 0, 0);
	add_new_instrument("EOI");
	add_new_inst_bag();
	add_new_inst_generator();
	add_new_inst_modulator();
	add_new_preset("EOP", 255, 255);
	add_new_preset_bag();
	add_new_preset_generator();
	add_new_preset_modulator();
}

//Add a brand new preset header to the list
void SF2::add_new_preset(const char *name, int patch, int bank)
{
	pdtalist_chunk->phdr_subchunk.add_preset(sfPresetHeader(this, name, patch, bank));
}

//Add a brand new instrument
void SF2::add_new_instrument(const char *name)
{
	pdtalist_chunk->inst_subchunk.add_instrument(sfInst(this, name));
}

//Add a new instrument bag to the instrument bag list
//DO NOT use this to add a preset bag !
void SF2::add_new_inst_bag()
{
	pdtalist_chunk->ibag_subchunk.add_bag(sfBag(this, false));
}

//Add a new preset bag to the preset bag list
//DO NOT use this to add an instrument bag !
void SF2::add_new_preset_bag()
{
	pdtalist_chunk->pbag_subchunk.add_bag(sfBag(this, true));
}

//Add a new modulator to the list
void SF2::add_new_preset_modulator()
{
	pdtalist_chunk->pmod_subchunk.add_modulator(sfModList(this));
}

//Add a new blank generator to the list
void SF2::add_new_preset_generator()
{
	pdtalist_chunk->pgen_subchunk.add_generator(sfGenList(this));
}

//Add a new customized generator to the list
void SF2::add_new_preset_generator(SFGenerator operation, uint16_t value)
{
	pdtalist_chunk->pgen_subchunk.add_generator(sfGenList(this, operation, genAmountType(value)));
}

//Add a new customized generator to the list
void SF2::add_new_preset_generator(SFGenerator operation, uint8_t lo, uint8_t hi)
{
	pdtalist_chunk->pgen_subchunk.add_generator(sfGenList(this, operation, genAmountType(lo, hi)));
}

//Add a new modulator to the list
void SF2::add_new_inst_modulator()
{
	pdtalist_chunk->imod_subchunk.add_modulator(sfModList(this));
}

//Add a new blank generator to the list
void SF2::add_new_inst_generator()
{
	pdtalist_chunk->igen_subchunk.add_generator(sfGenList(this));
}

//Add a new customized generator to the list
void SF2::add_new_inst_generator(SFGenerator operation, uint16_t value)
{
	pdtalist_chunk->igen_subchunk.add_generator(sfGenList(this, operation, genAmountType(value)));
}

//Add a new customized generator to the list
void SF2::add_new_inst_generator(SFGenerator operation, uint8_t lo, uint8_t hi)
{
	pdtalist_chunk->igen_subchunk.add_generator(sfGenList(this, operation, genAmountType(lo, hi)));
}

//Add a brand new header
void SF2::add_new_sample_header(const char *name, int start, int end, int start_loop, int end_loop, int sample_rate, int original_pitch, int pitch_correction)
{
	pdtalist_chunk->shdr_subchunk.add_sample(sfSample(this, name, start, end, start_loop, end_loop, sample_rate, original_pitch, pitch_correction));
}

// Add a new sample and create corresponding header
void SF2::add_new_sample(FILE *file, SampleType type, const char *name, uint32_t pointer, uint32_t size, bool loop_flag,
				  uint32_t loop_pos, uint32_t original_pitch, uint32_t pitch_correction, uint32_t sample_rate)
{
	uint32_t dir_offset = sdtalist_chunk->smpl_subchunk.add_sample(file, type, pointer, size, loop_flag, loop_pos);
	// If the sample is looped const SF2 standard requires we add the 8 bytes
	// at the start of the loop at the end (what a dumb standard)
	uint32_t dir_end, dir_loop_end, dir_loop_start;

	if (loop_flag)
	{
		dir_end = dir_offset + size + 8;
		dir_loop_end = dir_offset + size;
		dir_loop_start = dir_offset + loop_pos;
	}
	else
	{
		dir_end = dir_offset + size;
		dir_loop_end = 0;
		dir_loop_start = 0;
	}

	// Create sample header and add it to the list
	add_new_sample_header(name, dir_offset, dir_end, dir_loop_start, dir_loop_end, sample_rate, original_pitch, pitch_correction);
}

uint16_t SF2::get_ibag_size()
{
	return pdtalist_chunk->ibag_subchunk.bag_list.size();
}

uint16_t SF2::get_igen_size()
{
	return pdtalist_chunk->igen_subchunk.generator_list.size();
}

uint16_t SF2::get_imod_size()
{
	return pdtalist_chunk->imod_subchunk.modulator_list.size();
}

uint16_t SF2::get_pbag_size()
{
	return pdtalist_chunk->pbag_subchunk.bag_list.size();
}

uint16_t SF2::get_pgen_size()
{
	return pdtalist_chunk->pgen_subchunk.generator_list.size();
}

uint16_t SF2::get_pmod_size()
{
	return pdtalist_chunk->pmod_subchunk.modulator_list.size();
}