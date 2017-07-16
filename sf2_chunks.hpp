/**
 * Sound Font chunk classes
 * 
 * This is part of the GBA SoundFont Riper (c) 2012 by Bregalad
 * This is free and open source software.
 * 
 * Notes : I tried to separate the GBA-related stuff and SF2 related stuff as much as possible in different classes
 * that way anyone can re-use this program for building SF2s out of different data.
 * 
 * SF2 is a SoundFont file format. This class (and related classes) serves to create data to build
 * a SF2 file. For more details look at the document "Sound Font(R) Technical Documentation" version 2.01
 * which was used as a reference when writing these classes.
 * By the way I must tell this format is incredibly stupid and complex as opposed to what it could be and
 * I have no idea why people started to adopt this standard. But anyways they did...
 * 
 * This file contains classes for all chunks and subchunks present in a SF2 file
 * 
 * All SF2-related classes contains a field named sf2 that links to the SF2 main class to know which
 * SF2 file they relates to. (this would make it possible to build multiple SF2 files at a time).
 * 
 * Note : These classes helps in the complex process of building a SF2 and make it much more simpler and automated.
 * Not all of SF2's features are supported, support for ROM samples, stereo samples and modulators is
 * incomplete Yet those classes contains nothing GBA-related and could easily be re-used in another project as-it.
 * 
 * There is basically 2 ways to adds data, such as a new sample, new instrument of new preset.
 * 
 * The first way is to make it all automatically by using the add_new_xxx() functions. It's recommended to do
 * it this way whenever possible HOWEVER to have a file which is structurally correct it is imperative to
 * add them in this order :
 * 1) Instrument/Preset header
 * 2) Bag 1
 * 3) Modulators and generators for Bag1
 * 4) Bag 2 (optional)
 * 5) Modulators and generators for Bag2 (optional)
 * etc...
 * 
 * The second way is to create moderators, generators and bag directly using their respective classes. That way
 * they can be created in any order, but it's important they are created in a first step, and added in a second step
 * in the order they should be in the final file.
 */
#ifndef SF2_CHUNKS_HPP
#define SF2_CHUNKS_HPP

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "sf2.hpp"
#include "sf2_types.hpp"
#include <vector>

// SF2Chunks abstract class
// All Chunks and SubChunks should extend this class.
// They should all call this constructor first,
// and they should have a write() function that calls this write() function first.
class SF2Chunks
{
protected:
	char name[4];		// 4-letter name of the subchunk
public:
	uint32_t size;		// Size in bytes of the subchunk
protected:
	SF2 *sf2;

	// Constructor (should be systematically called by sub-classes)
	SF2Chunks(SF2 *sf2, const char name[4], uint32_t size = 0) :
		size(size),			// The chunk starts bank
		sf2(sf2)			// Link to output SF2 file
	{
		for(unsigned int i=0; i < 4; ++i)
			SF2Chunks::name[i] = name[i];
	}

	// Write the name and size of the (sub)chunk (should be systematically called by sub-classes)
	inline void write()
	{
		fwrite(&name, 2, 4, sf2->out);
	}
};


/*
 * Helper classes
 */

// Preset header class
class sfPresetHeader
{
	char ach_preset_name[20];			// Preset's name
	uint16_t wPreset;					// Patch #
	uint16_t wBank;						// Bank #
	uint16_t wPresetBagNdx;				// Index to "bag" of instruments (private - created automatically)
	const uint32_t dwLibrary = 0;		// Unused values - should be kept to 0
	const uint32_t dwGenre = 0;
	const uint32_t dwMorphology = 0;
	SF2 *sf2;
public:
	sfPresetHeader(SF2 *sf2, const char *name, uint16_t patch, uint16_t bank) :
		wPreset(patch), wBank(bank), sf2(sf2)
	{
		strncpy(ach_preset_name, name, 20);
		wPresetBagNdx = sf2->get_pbag_size();
	}

	void write()
	{
		fwrite(&ach_preset_name, 1, 38, sf2->out);
	}
}__attribute__ ((packed));

// Preset bag class
class sfBag
{
	// Private to prevent anyone from affecting the
	// indexes which are automatically created....
	uint16_t wGenNdx;			// Index to list of generators
	uint16_t wModNdx;			// Index to list of modulators
	SF2 *sf2;
public:
	// Automatically assign indexes 
	sfBag(SF2 *sf2, bool preset) :
		sf2(sf2)
	{
		if(preset)
		{
			wGenNdx = sf2->get_pgen_size();
			wModNdx = sf2->get_pmod_size();
		}
		else
		{
			wGenNdx = sf2->get_igen_size();
			wModNdx = sf2->get_imod_size();
		}
	}

	void write()
	{
		fwrite(&wGenNdx, 2, 2, sf2->out);
	}
};

// Modulator class (mostly unused)
class sfModList
{
	SFModulator sfModSrcOper;		// Modulator source
	SFGenerator sfModDestOper;		// Modulator destination
	uint16_t modAmount;				// Modulator value
	SFModulator sfModAmtSrcOper;	// Modulator source ??
	SFTransform sfModTransOper;		// Transformation curvative
	SF2 *sf2;
public:
	sfModList(SF2 *sf2) :
		sfModSrcOper(SFModulator::_null),
		sfModDestOper(SFGenerator::_null),
		modAmount(0),
		sfModAmtSrcOper(SFModulator::_null),
		sfModTransOper(SFTransform::_null),
		sf2(sf2)
	{}

	void write()
	{
		fwrite(&sfModSrcOper, 2, 5, sf2->out);
	}
};

// Generator class
// This is extremely important
class sfGenList
{
	SFGenerator sfGenOper;
	genAmountType genAmount; 
	SF2 *sf2;
public:
	sfGenList(SF2 *sf2) :
		sfGenOper(SFGenerator::_null), sf2(sf2)
	{
		genAmount.shAmount = 0;
	}
	// Straightforward constructor
	sfGenList(SF2 *sf2, SFGenerator operation, genAmountType amount) :
		sfGenOper(operation), genAmount(amount), sf2(sf2)
	{}

	void write()
	{
		fwrite(&sfGenOper, 2, 2, sf2->out);
	}
};

// Instrument zone class
class sfInst
{
	char achInstName[20];
	uint16_t wInstBagNdx;
	SF2 *sf2;
public:
	// Constructor that automatically points at the end of the (current) preset bag
	sfInst(SF2 *sf2, const char *name) : sf2(sf2)
	{
		strncpy(achInstName, name, 20);
		wInstBagNdx = sf2->get_ibag_size();
	}

	void write()
	{
		fwrite(&achInstName, 1, 22, sf2->out);
	}
};

class sfSample
{
	char achSampleName[20];
	uint32_t dwStart;
	uint32_t dwEnd;
	uint32_t dwStartloop;
	uint32_t dwEndloop;
	uint32_t dwSampleRate;
	int8_t byOriginalPitch;
	int8_t chPitchCorrection;
	uint16_t wSampleLink;
	SFSampleLink sfSampleType;
	SF2 *sf2;
public:
	sfSample(SF2 *sf2, const char *name, uint32_t start, uint32_t end, uint32_t start_loop, uint32_t end_loop, uint32_t sample_rate, int8_t original_pitch, int8_t pitch_correction) :
		dwStart(start),
		dwEnd(end),
		dwStartloop(start_loop),
		dwEndloop(end_loop),
		dwSampleRate(sample_rate),
		byOriginalPitch(original_pitch),
		chPitchCorrection(pitch_correction),
		wSampleLink(0),
		sfSampleType(SFSampleLink::monoSample),
		sf2(sf2)
	{
		strncpy(achSampleName, name, 20);
	}

	void write()
	{
		fwrite(&achSampleName, 1, 46, sf2->out);
	}
};

/* sub-chunk classes
 *
 * Those are chunks withing a chunk
 */

// Version sub-chunk
class IFILSubChunk : public SF2Chunks
{
	// Output format is SoundFont v2.1
	const uint16_t wMajor;
	const uint16_t wMinor;
public:
	IFILSubChunk(SF2 *sf2) :
		SF2Chunks(sf2, "ifil", 4),
		wMajor(2),
		wMinor(1)
	{}

	void write()
	{	// Write major and minor revision format
		SF2Chunks::write();
		fwrite(&wMajor, 2, 2, sf2->out);
	}
};

// Class for the various header chunks that just contain a string
class HeaderSubChunk : public SF2Chunks
{
	const char *field;
public:
	HeaderSubChunk(SF2 *sf2, const char *subchunk_type, const char *s) :
		SF2Chunks(sf2, subchunk_type, strlen(s)+1), field(s)
	{}		// The string is null terminated -> it takes one more byte

	void write()
	{
		SF2Chunks::write();
		fwrite(field, 1, size, sf2->out);	// Write the string followed by a null byte
	}
};

// Class for the samples sub chunk
class SMPLSubChunk : public SF2Chunks
{
	// To prevent the program from using a lot of memory by caching all
	// samples before writing them (which is not useful)
	// I instead store a list of pointers to sample data, and the data
	// is directly read from the original file when the sample should
	// be written to output

	// I made this function as generic as possible, so any sample can be loaded
	// from any file, in various formats.

	std::vector<FILE*> file_list;				// Files from which the samples must be read
	std::vector<uint32_t> pointer_list;			// address within files where the samples must be read
	std::vector<uint32_t> size_list;			// Size of the data sample
	std::vector<bool> loop_flag_list;			// Loop flag for samples (required as we need to copy data after the loop)
	std::vector<uint32_t> loop_pos_list;		// Loop start data (irrelevent if loop flag is clear - add dummy data)
	std::vector<SampleType> sample_type_list;	// Type of sample (unsigned / signed, 8/16 bits etc...)

public:
	SMPLSubChunk(SF2 *sf2) :
		SF2Chunks(sf2, "smpl")
	{}

	// Add a sample to the package
	// Returns directory index of the start of the sample
	uint32_t add_sample(FILE *file, SampleType type, uint32_t pointer, uint32_t size, bool loop_flag, uint32_t loop_pos)
	{
		file_list.push_back(file);
		pointer_list.push_back(pointer);
		size_list.push_back(size);
		loop_flag_list.push_back(loop_flag);
		loop_pos_list.push_back(loop_pos);
		sample_type_list.push_back(type);

		uint32_t dir_offset = SF2Chunks::size >> 1;
		// 2 bytes per sample
		// Compute size including the 8 samples after loop point
		// and 46 dummy samples
		if(loop_flag)
			SF2Chunks::size += (size + 8 + 46) * 2;
		else
			SF2Chunks::size += (size + 46) * 2;

		return dir_offset;
	}

	// Write all samples to output in little Indian format
	void write()
	{
		SF2Chunks::write();

		for(unsigned int i=0; i<file_list.size(); i++)
		{
			// Seek at the start of the sample in input file
			fseek(file_list[i], pointer_list[i], SEEK_SET);

			// Using a cached buffer really speeds up the writing process a lot !!
			int16_t *outbuf = new int16_t[size_list[i]];

			switch (sample_type_list[i])
			{
				// Source is unsigned 8 bits
				case UNSIGNED_8:
				{
					uint8_t *data = new uint8_t[size_list[i]];
					fread(data, 1, size_list[i], file_list[i]);
					// Convert to signed 16 bits
					for(unsigned int j=0; j < size_list[i]; j++)
						outbuf[j] = (data[j] - 0x80) << 8;
					delete[] data;
				}	break;

				// Source is signed 8 bits
				case SIGNED_8:
				{
					int8_t *data = new int8_t[size_list[i]];
					fread(data, 1, size_list[i], file_list[i]);

					for(unsigned int j=0; j < size_list[i]; j++)
						outbuf[j] = data[j] << 8;
					delete[] data;
				}	break;

				case SIGNED_16:
					// Just read raw data, no conversion needed
					fread(outbuf, 2, size_list[i], file_list[i]);
					break;
				
				case GAMEBOY_CH3:
				{
					// Conversion lookup table
					const int16_t conv_tbl[] =
					{
						-0x4000, -0x3800, -0x3000, -0x2800, -0x2000, -0x1800, -0x0100, -0x0800,
						0x0000, 0x0800, 0x1000, 0x1800, 0x2000, 0x2800, 0x3000, 0x3800
					};

					int num_of_repts = size_list[i]/32;
					// Data is always on 16 bytes
					uint8_t data[16];
					fread(data, 1, 16, file_list[i]);

					for(int j=0, l=0; j<16; j++)
					{
						for(int k=num_of_repts; k!=0; k--, l++)
							outbuf[l] = conv_tbl[data[j]>>4];
	
						for(int k=num_of_repts; k!=0; k--, l++)
							outbuf[l] = conv_tbl[data[j]&0xf];
					}
				}	break;
				
				case BDPCM:
				{
					static const int8_t delta_lut[] = {0, 1, 4, 9, 16, 25, 36, 49, -64, -49, -36, -25, -16, -9, -4, -1};

					/*
					 * A block consists of an initial signed 8 bit PCM byte
					 * followed by 63 nibbles stored in 32 bytes.
					 * The first of these bytes has a zero padded (unused) high nibble.
					 * This makes up of a total block size of 65 (0x21) bytes each.
					 *
					 * Decoding works like this:
					 * The initial byte can be directly read without decoding. Then each
					 * next sample can be decoded by putting the nibble into the delta-lookup-table 
					 * and adding that value to the previously calculated sample
					 * until the end of the block is reached.
					 */

					unsigned int nblocks = size_list[i] / 64;		// 64 samples per block

					char (*data)[33] = new char[nblocks][33];
					fread(data, 33, nblocks, file_list[i]);

					for(unsigned int block=0; block < nblocks; ++block)
					{
						int8_t sample = data[block][0];
						outbuf[64*block] = sample << 8;
						sample += delta_lut[data[block][1] & 0xf];
						outbuf[64*block+1] = sample << 8;
						for (unsigned int j = 1; j < 32; ++j)
						{
							uint8_t d = data[block][j+1];
							sample += delta_lut[d >> 4];
							outbuf[64*block+2*j] = sample << 8;
							sample += delta_lut[d & 0xf];
							outbuf[64*block+2*j+1]= sample << 8;
						}
					}
					memset(outbuf+64*nblocks, 0, size_list[i]-64*nblocks);		// Remaining samples are always 0

					delete[] data;
				}   break;
			}

			// Write buffer
			fwrite(outbuf, 2, size_list[i], sf2->out);

			// If loop enabled, write 8 samples after loop point
			// (required by the dumb SF2 standard)
			if(loop_flag_list[i])
				fwrite(outbuf + loop_pos_list[i], 2, 8, sf2->out);

			// Write 46 dummy zeroed samples at the end
			// which is also required by the very dumb SF2 standard
			for(int j = 0; j < 2*46; j++)
				putc(0x00, sf2->out);

			delete[] outbuf;
		}
	}
};

// Preset header list sub-chunk
class PHDRSubChunk : public SF2Chunks
{
	std::vector<sfPresetHeader> preset_list;
public:
	PHDRSubChunk(SF2 *sf2) :		// Init size and name
		SF2Chunks(sf2, "phdr")
	{}

	// Add an existing preset header to the list
	void add_preset(const sfPresetHeader& preset)
	{
		preset_list.push_back(preset);
		size += 38;				// Each entry is exactly 38 bytes long
	}

	void write()
	{
		SF2Chunks::write();

		// Call the write function for all elements of the list
		for(unsigned int i=0; i<preset_list.size(); i++)
			preset_list[i].write();
	}
};

// Instrument list sub chunk
class INSTSubChunk : public SF2Chunks
{
	std::vector<sfInst> instrument_list;
public:
	INSTSubChunk (SF2 *sf2) :
		SF2Chunks(sf2, "inst")
	{}

	// Add an existing instrument
	void add_instrument(const sfInst& instrument)
	{
		instrument_list.push_back(instrument);
		size += 22;
	}

	void write()
	{
		SF2Chunks::write();

		for(unsigned int i=0; i<instrument_list.size(); i++)
			instrument_list[i].write();
	}
};

// Preset/Instrument bag list sub chunk
class BAGSubChunk : public SF2Chunks
{
	std::vector<sfBag> bag_list;
	friend uint16_t SF2::get_ibag_size();
	friend uint16_t SF2::get_pbag_size();

public:
	BAGSubChunk (SF2 *sf2, bool preset) :		// Init size and name
		SF2Chunks(sf2, preset ? "pbag" : "ibag")
	{}

	// Add an existing bag to the list
	void add_bag(const sfBag& bag)
	{
		bag_list.push_back(bag);
		size += 4;				// Each entry is exactly 4 bytes long
	}

	void write()
	{
		SF2Chunks::write();

		// Call the write function for all elements of the list
		for(unsigned int i=0; i<bag_list.size(); i++)
			bag_list[i].write();
	}
};

// Preset/Instrument Modulator list class
class MODSubChunk : public SF2Chunks
{
	std::vector<sfModList> modulator_list;
	friend uint16_t SF2::get_imod_size();
	friend uint16_t SF2::get_pmod_size();

public:
	MODSubChunk (SF2 *sf2, bool preset) :
		SF2Chunks(sf2, preset ? "pmod" : "imod")
	{
		// if(preset)
			// name = "pmod";
		// else
			// name = "imod";
	}

	// Add an existing modulator to the list
	void add_modulator(const sfModList& modulator)
	{
		modulator_list.push_back(modulator);
		size += 10;
	}

	void write()
	{
		SF2Chunks::write();

		for(unsigned int i=0; i<modulator_list.size(); i++)
			modulator_list[i].write();
	}
};

// Preset/Instrument Generator list class
class GENSubChunk : public SF2Chunks
{
	std::vector<sfGenList> generator_list;
	friend uint16_t SF2::get_igen_size();
	friend uint16_t SF2::get_pgen_size();

public:
	GENSubChunk (SF2 *sf2, bool preset) :
		SF2Chunks(sf2, preset ? "pgen" : "igen")
	{
		// if(preset)
			// name = "pgen";
		// else
			// name = "igen";
	}

	// Add an existing generator to the list
	void add_generator(const sfGenList& generator)
	{
		generator_list.push_back(generator);
		size += 4;
	}

	void write()
	{
		SF2Chunks::write();

		for(unsigned int i=0; i<generator_list.size(); i++)
			generator_list[i].write();
	}
};

// Sample header list class
class SHDRSubChunk : public SF2Chunks
{
	std::vector<sfSample> sample_list;
public:
	SHDRSubChunk(SF2 *sf2) :
		SF2Chunks(sf2, "shdr")
	{}

	// Add an existing header to the list
	void add_sample(const sfSample& sample)
	{
		sample_list.push_back(sample);
		size += 46;
	}

	void write()
	{
		SF2Chunks::write();

		for(unsigned int i=0; i<sample_list.size(); i++)
			sample_list[i].write();
	}
};


/*
 * Now we have the actual 3 main chunks, mades of several subchunks
 */

// Info List chunk, containing info about the SF2 file
class InfoListChunk : public SF2Chunks
{
	// Sub-chunks in InfoList chunk
	IFILSubChunk ifil_subchunk;
	HeaderSubChunk isng_subchunk;
	HeaderSubChunk inam_subchunk ;
	HeaderSubChunk ieng_subchunk;
	HeaderSubChunk icop_subchunk;
public:
	// Info List constructor
	InfoListChunk(SF2 *sf2) :
		SF2Chunks(sf2, "LIST"),
		ifil_subchunk(sf2),
		isng_subchunk(sf2, "isng", "EMU8000"),
		inam_subchunk(sf2, "INAM", "Unnamed"),
		ieng_subchunk(sf2, "IENG", "Nintendo Game Boy Advance SoundFont"),
		icop_subchunk(sf2, "ICOP", "Ripped with SF2Ripper v0.0 (c) 2012 by Bregalad")
	{}

	// Compute size of the info-list chunk
	uint32_t calcSize()
	{
		size = 4;
		size += ifil_subchunk.size + 8;
		size += isng_subchunk.size + 8;
		size += inam_subchunk.size + 8;
		size += ieng_subchunk.size + 8;
		size += icop_subchunk.size + 8;
		return size;
	}

	void write()
	{
		SF2Chunks::write();				// Write chunk name and size
		fwrite("INFO", 1, 4, sf2->out);	// Chunk header

		ifil_subchunk.write();			// Write all 5 sub-chunk elements
		isng_subchunk.write();
		inam_subchunk.write();
		ieng_subchunk.write();
		icop_subchunk.write();
	}
};

// Sample data list chunk, contains samples
class SdtaListChunk : public SF2Chunks
{
	SMPLSubChunk smpl_subchunk;

	friend void SF2::add_new_sample(FILE *file, SampleType type, const char *name, uint32_t pointer, uint32_t size, bool loop_flag,
				  uint32_t loop_pos, uint32_t original_pitch, uint32_t pitch_correction, uint32_t sample_rate);
public:
	SdtaListChunk (SF2 *sf2) :
		SF2Chunks(sf2, "LIST"),
		smpl_subchunk(sf2)
	{}

	// Compute size of sample-data-list chunk
	uint32_t calcSize()
	{
		size = 4;
		size += smpl_subchunk.size + 8;
		return size;
	}

	void write()
	{
		SF2Chunks::write();
		fwrite("sdta", 1, 4, sf2->out);

		smpl_subchunk.write();
	}
};

// Hydra chunk, contains data for instruments, presets and samples header
class HydraChunk : public SF2Chunks
{
	// Sub chunks in PTDA chunk
	PHDRSubChunk phdr_subchunk;
	BAGSubChunk pbag_subchunk;
	MODSubChunk pmod_subchunk;
	GENSubChunk pgen_subchunk;
	INSTSubChunk inst_subchunk;
	BAGSubChunk ibag_subchunk;
	MODSubChunk imod_subchunk;
	GENSubChunk igen_subchunk;
	SHDRSubChunk shdr_subchunk;

	friend void SF2::add_new_preset(const char *name, int patch, int bank);
	friend void SF2::add_new_instrument(const char *name);
	friend void SF2::add_new_inst_bag();
	friend void SF2::add_new_preset_bag();
	friend void SF2::add_new_preset_modulator();
	friend void SF2::add_new_preset_generator();
	friend void SF2::add_new_preset_generator(SFGenerator operation, uint16_t value);
	friend void SF2::add_new_preset_generator(SFGenerator operation, uint8_t lo, uint8_t hi);
	friend void SF2::add_new_inst_modulator();
	friend void SF2::add_new_inst_generator();
	friend void SF2::add_new_inst_generator(SFGenerator operation, uint16_t value);
	friend void SF2::add_new_inst_generator(SFGenerator operation, uint8_t lo, uint8_t hi);
	friend void SF2::add_new_sample_header(const char *name, int start, int end, int start_loop, int end_loop, int sample_rate, int original_pitch, int pitch_correction);

	friend uint16_t SF2::get_ibag_size();
	friend uint16_t SF2::get_imod_size();
	friend uint16_t SF2::get_igen_size();
	friend uint16_t SF2::get_pbag_size();
	friend uint16_t SF2::get_pmod_size();
	friend uint16_t SF2::get_pgen_size();

public:
	// Constructor
	HydraChunk(SF2 *sf2) :
		SF2Chunks(sf2, "LIST"),
		phdr_subchunk(sf2),
		pbag_subchunk(sf2, true),
		pmod_subchunk(sf2, true),
		pgen_subchunk(sf2, true),
		inst_subchunk(sf2),
		ibag_subchunk(sf2, false),
		imod_subchunk(sf2, false),
		igen_subchunk(sf2, false),
		shdr_subchunk(sf2)
	{}

	uint32_t calcSize()
	{
		size = 4;
		// Compute size of the chunk (because we don't know it in advance)
		size += phdr_subchunk.size + 8;
		size += pbag_subchunk.size + 8;
		size += pmod_subchunk.size + 8;
		size += pgen_subchunk.size + 8;
		size += inst_subchunk.size + 8;
		size += ibag_subchunk.size + 8;
		size += imod_subchunk.size + 8;
		size += igen_subchunk.size + 8;
		size += shdr_subchunk.size + 8;

		return size;
	}

	void write()
	{
		// Write chunk name and size
		SF2Chunks::write();
		fwrite("pdta", 1, 4, sf2->out);

		// Write all sub-chunks
		phdr_subchunk.write();
		pbag_subchunk.write();
		pmod_subchunk.write();
		pgen_subchunk.write();
		inst_subchunk.write();
		ibag_subchunk.write();
		imod_subchunk.write();
		igen_subchunk.write();
		shdr_subchunk.write();
	}
};

#endif