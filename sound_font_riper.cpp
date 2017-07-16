/*
 * GBA Sound Font Riper (c) 2012, 2014 by Bregalad
 * This is free and open source software.
 * 
 * This program extracts soundfont data from a GBA game using
 * Nintendo's "sappy" engine (which ~90% of commercial GBA games are using),
 * and converts it to the widely used SF2 Sound Font format.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "sf2.hpp"
#include "gba_instr.hpp"
#include "hex_string.hpp"
#include <set>

static FILE *outSF2;
static FILE *out_txt = stdout;		// Log on stdout by default

// Global variables
FILE *psg_data;
FILE *goldensun_synth;
FILE *inGBA;

static bool verbose_flag = false;
static bool verbose_output_to_file = false;
static bool change_sample_rate = false;
static bool gm_preset_names = false;

static unsigned int sample_rate = 22050;
static std::set<uint32_t> addresses;
static unsigned int current_address;
static unsigned int current_bank;
static unsigned int current_instrument;
static unsigned int main_volume = 15;

static SF2 *sf2;
static GBAInstr *instruments;

static void print_instructions()
{
	puts
	(
		"Dumps a sound bank (or a list of sound banks) from a GBA game which is using the sappy sound engine to SoundFont 2.0 (.sf2) format\n"
		"Usage : sound_font_riper [options] in.gba out.sf2 address1 [address2] ...\n"
		"addresses will correspond to instrument banks in increasing order...\n"
		"Available options :\n"
		"-v : verbose : Display info about the sound font in text format. If -v is followed by a file name,\n"
		"     info is output to the specified file instead.\n"
		"-s : Sampling rate for samples. Default : 22050 Hz\n"
		"-gm : Give General MIDI names to presets. Note that this will only change the names and will NOT\n"
		"      magically turn the soundfont into a General MIDI compliant soundfont.\n"
		"-mv : Main volume for sample instruments. Range : 1-15. Game Boy channels are unnaffected.\n"
	);
	exit(0);
}


// General MIDI instrument names
static const char *const general_MIDI_instr_names[128] =
{
	"Acoustic Grand Piano", "Bright Acoustic Piano", "Electric Grand Piano", "Honky-tonk Piano", "Rhodes Piano", "Chorused Piano",
	"Harpsichord",	"Clavinet", "Celesta", "Glockenspiel", "Music Box", "Vibraphone", "Marimba", "Xylophone", "Tubular Bells", "Dulcimer",
	"Hammond Organ", "Percussive Organ", "Rock Organ", "Church Organ", "Reed Organ", "Accordion", "Harmonica", "Tango Accordion",
	"Acoustic Guitar (nylon)", "Acoustic Guitar (steel)", "Electric Guitar (jazz)", "Electric Guitar (clean)", "Electric Guitar (muted)",
	"Overdriven Guitar", "Distortion Guitar", "Guitar Harmonics", "Acoustic Bass", "Electric Bass (finger)", "Electric Bass (pick)",
	"Fretless Bass", "Slap Bass 1", "Slap Bass 2", "Synth Bass 1", "Synth Bass 2", "Violin", "Viola", "Cello", "Contrabass",
	"Tremelo Strings", "Pizzicato Strings", "Orchestral Harp", "Timpani", "String Ensemble 1", "String Ensemble 2", "SynthStrings 1",
	"SynthStrings 2", "Choir Aahs", "Voice Oohs", "Synth Voice", "Orchestra Hit", "Trumpet", "Trombone", "Tuba", "Muted Trumpet",
	"French Horn", "Brass Section", "Synth Brass 1", "Synth Brass 2", "Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax",
	"Oboe", "English Horn", "Bassoon", "Clarinet", "Piccolo", "Flute", "Recorder", "Pan Flute", "Bottle Blow", "Shakuhachi", "Whistle",
	"Ocarina", "Lead 1 (square)", "Lead 2 (sawtooth)", "Lead 3 (calliope lead)", "Lead 4 (chiff lead)", "Lead 5 (charang)",
	"Lead 6 (voice)", "Lead 7 (fifths)", "Lead 8 (bass + lead)", "Pad 1 (new age)", "Pad 2 (warm)", "Pad 3 (polysynth)", "Pad 4 (choir)",
	"Pad 5 (bowed)", "Pad 6 (metallic)", "Pad 7 (halo)", "Pad 8 (sweep)", "FX 1 (rain)", "FX 2 (soundtrack)", "FX 3 (crystal)",
	"FX 4 (atmosphere)", "FX 5 (brightness)", "FX 6 (goblins)",	"FX 7 (echoes)", "FX 8 (sci-fi)", "Sitar", "Banjo", "Shamisen", "Koto",
	"Kalimba", "Bagpipe", "Fiddle", "Shanai", "Tinkle Bell", "Agogo", "Steel Drums", "Woodblock", "Taiko Drum", "Melodic Tom",
	"Synth Drum", "Reverse Cymbal", "Guitar Fret Noise", "Breath Noise", "Seashore", "Bird Tweet", "Telephone Ring", "Helicopter",
	"Applause", "Gunshot"
};

// Add initial attenuation preset to balance between GameBoy and sampled instruments
static void add_attenuation_preset()
{
	if(main_volume < 15)
	{
		const uint16_t attenuation = uint16_t(100.0 * log(15.0/main_volume));
		sf2->add_new_preset_generator(SFGenerator::initialAttenuation, attenuation);
	}
}

// Convert a GBA instrument in its SF2 counterpart
// if any kind of error happens, it will do nothing and exit
static void build_instrument(const inst_data inst)
{
	uint8_t instr_type = inst.word0 & 0xff;
	std::string name;
	if(gm_preset_names)
		name = std::string(general_MIDI_instr_names[current_instrument]);
	else
		// (poetic) name of the SF2 preset...
		name = "Type " + std::to_string(instr_type) + " @0x" + hex(current_address);

	try
	{
		switch (instr_type)
		{	// Sampled instrument types
			case 0x00:
			case 0x08:
			case 0x10:
			case 0x18:
			case 0x20:
			case 0x28:
			case 0x30:
			case 0x38:
			{
				int i = instruments->build_sampled_instrument(inst);
				sf2->add_new_preset(name.c_str(), current_instrument, current_bank);
				sf2->add_new_preset_bag();
				// Add initial attenuation preset to balance volume between sampled and GB instruments
				add_attenuation_preset();
				sf2->add_new_preset_generator(SFGenerator::instrument, i);
			}	break;

			// GameBoy pulse wave instruments
			case 0x01:
			case 0x02:
			case 0x09:
			case 0x0a:
			{
				// Can only convert them if the psg_data file is found
				if(psg_data)
				{
					int i = instruments->build_pulse_instrument(inst);
					sf2->add_new_preset(name.c_str(), current_instrument, current_bank);
					sf2->add_new_preset_bag();
					sf2->add_new_preset_generator(SFGenerator::instrument, i);
				}
			}	break;

			// GameBoy channel 3 instrument
			case 0x03:
			case 0x0b:
			{
				int i = instruments->build_GB3_instrument(inst);
				sf2->add_new_preset(name.c_str(), current_instrument, current_bank);
				sf2->add_new_preset_bag();
				sf2->add_new_preset_generator(SFGenerator::instrument, i);
			}	break;

			// GameBoy noise instruments, not supported yet
			case 0x04:
			case 0x0c:
			{
				if(psg_data)
				{
					int i = instruments->build_noise_instrument(inst);
					sf2->add_new_preset(name.c_str(), current_instrument, current_bank);
					sf2->add_new_preset_bag();
					sf2->add_new_preset_generator(SFGenerator::instrument, i);
				}
			}	break;

			// Key split instrument
			case 0x40:
			{
				int i = instruments->build_keysplit_instrument(inst);
				sf2->add_new_preset(name.c_str(), current_instrument, current_bank);
				sf2->add_new_preset_bag();
				// Add initial attenuation preset to balance volume between sampled and GB instruments
				add_attenuation_preset();
				sf2->add_new_preset_generator(SFGenerator::instrument, i);
			}	break;

			// Every key split instrument
			case 0x80:
			{
				int i = instruments->build_every_keysplit_instrument(inst);
				sf2->add_new_preset(name.c_str(), current_instrument, current_bank);
				sf2->add_new_preset_bag();
				// Add initial attenuation preset to balance volume between sampled and GB instruments
				add_attenuation_preset();
				sf2->add_new_preset_generator(SFGenerator::instrument, i);
			}	break;

			// Ignore other instrument types
			default:
				break;
		}

		// If there is any error in the process just ignore it and silently continue
		// In fact dozen of errors always happened all the times so I removed any form of error messages
	}
	catch (...)
	{}
}

// Display verbose to console or output to file if requested
static void print(const std::string& s)
{
	if(verbose_flag)
		fprintf(out_txt, s.c_str());
}

static void print(const char* s)
{
	if(verbose_flag)
		fprintf(out_txt, s);
}

// Display ADSR values used
static void adsr(uint32_t adsr)
{
	int attack = adsr & 0xFF;
	int decay = (adsr>>8) & 0xFF;
	int sustain = (adsr>>16) & 0xFF;
	int release = adsr>>24;
	// Print ADSR values
	fprintf(out_txt, "      ADSR : %d, %d, %d, %d\n", attack, decay, sustain, release);
}

// Display duty cycle used
static void duty_cycle(int duty)
{
	const char *const cycles[4] = {"12.5%", "25%", "50%", "75%"};
	fprintf(out_txt, "      Duty cycle : %s\n", cycles[duty&3]);
}

// This function read instrument data and outputs info on the screen or on the verbose file
// it's not actually needed to convert the data to SF2 format, but is very useful for debugging
static void verbose_instrument(const inst_data inst, bool recursive)
{
	// Do nothing with unused instruments
	if(inst.word0 == 0x3c01 && inst.word1 == 0x02 && inst.word2 == 0x0F0000) return;

	uint8_t instr_type = inst.word0 & 0xff;
	fprintf(out_txt, "  Type : 0x%x  ", instr_type);
	switch(instr_type)
	{	
		// Sampled instruments
		case 0 :
		case 8 :
		case 0x10 :
		case 0x18 :
		case 0x20 :
		case 0x28 :
		case 0x30 :
		case 0x38 :
		{
			uint32_t sadr = inst.word1 & 0x3ffffff;
			fprintf(out_txt, "(sample @0x%x)\n", sadr);

			try
			{
				if(fseek(inGBA, sadr, SEEK_SET)) throw -1;
				struct
				{
					uint32_t loop;
					uint32_t pitch;
					uint32_t loop_pos;
					uint32_t len;
				}
				ins;
				fread(&ins, 4, 4, inGBA);

				fprintf(out_txt, "      Pitch : %u\n", ins.pitch/1024);
				fprintf(out_txt, "      Length : %u\n", ins.len);

				if(ins.loop == 0)
					fputs("      Not looped\n", out_txt);
				else if(ins.loop == 0x40000000)
					fprintf(out_txt, "      Loop enabled at : %u\n", ins.loop_pos);
				else if(ins.loop == 0x1)
					fputs("      BDPCM compressed\n", out_txt);
				else
					fputs("      Unknown loop type\n", out_txt);

				adsr(inst.word2);
			}
			catch (...)
			{
				fputs("Invalid instrument (an exception occured)", out_txt);
			}
		}	break;

		// Pulse channel 1 instruments
		case 1 :
		case 9 :
		{
			fputs("(GB pulse channel 1)", out_txt);
			if((char)inst.word0 != 8)			// Display sweep if enabled on GB channel 1
				fprintf(out_txt, "      Sweep : 0x%x\n", inst.word0 & 0xFF);

			adsr(inst.word2);
			duty_cycle(inst.word1);
		}	break;
		
		// Pulse channel 2 instruments
		case 2 :
		case 10 :
		case 18 :
		{
			fputs("(GB pulse channel 2)", out_txt);
			adsr(inst.word2);
			duty_cycle(inst.word1);
		}	break;

		// Channel 3 instruments
		case 3 :
		case 11 :
		{
			fputs("(GB channel 3)", out_txt);
			adsr(inst.word2);
			fputs("      Waveform : ", out_txt);

			try
			{
				// Seek to waveform's location
				if(fseek(inGBA, inst.word1&0x3ffffff, SEEK_SET)) throw -1;
				int waveform[32];

				for(int j=0; j<16; j++)
				{
					uint8_t a = fgetc(inGBA);
					waveform[2*j] = a>>4;
					waveform[2*j+1] = a & 0xF;
				}

				// Display waveform in text format
				for(int j=7; j>=0; j--)
				{
					for(int k=0; k!=32; k++)
					{
						if(waveform[k] == 2*j)
							fputc('_', out_txt);
						else if(waveform[k] == 2*j+1)
							fputc('-', out_txt);
						else
							fputc(' ', out_txt);
					}
					fputc('\n', out_txt);
				}
			}
			catch(...)
			{
				fputs("Invalid instrument (an exception occured)", out_txt);
			}
		}	break;

		// Noise instruments
		case 4 :
		case 12 :
			fputs("(GB noise channel 4)", out_txt);
			adsr(inst.word2);
			if(inst.word1  == 0)
				fputs("      long random sequence\n", out_txt);
			else
				fputs("      short random sequence\n", out_txt);
			break;

		// Key-split instruments
		case 0x40 :
			fputs("Key-split instrument", out_txt);

			if(!recursive)
			{
				bool *keys_used = new bool[128]();
				try
				{
				// seek to key table's location
					if(fseek(inGBA, inst.word2&0x3ffffff, SEEK_SET)) throw -1;

					for(int k = 0; k!= 128; k++)
					{
						uint8_t c = fgetc(inGBA);
						if(c & 0x80) continue;		// Ignore entries with MSB set (invalid)
						keys_used[c] = true;
					}

					int instr_table = inst.word1 & 0x3ffffff;

					for(int k = 0; k!= 128; k++)
					{
						// Decode instruments used at least once in the key table
						if(keys_used[k])
						{
							try
							{
								// Seek to the addressed instrument
								if(fseek(inGBA, instr_table + 12*k, SEEK_SET)) throw -1;
								inst_data sub_instr;
								// Read the addressed instrument
								fread(&sub_instr, 4, 3, inGBA);
	
								fprintf(out_txt, "\n      Sub_intrument %d", k);
								verbose_instrument(sub_instr, true);
							}
							catch(...)
							{
								fputs("Invalid sub-instrument (an exception occurred)", out_txt);
							}
						}
					}
				}
				catch (...)
				{}
				delete[] keys_used;
			}
			else
				fputs("   Illegal double-recursive instrument !", out_txt);
			break;

		// Every key split instruments
		case 0x80 :
			fputs("Every key split instrument", out_txt);

			if(!recursive)
			{
				uint32_t address = inst.word1 & 0x3ffffff;
				for(int k = 0; k<128; ++k)
				{
					try
					{
						if(fseek(inGBA, address + k*12, SEEK_SET)) throw -1;
						inst_data key_instr;
						fread(&key_instr, 4, 3, inGBA);
						
						fprintf(out_txt, "\n   Key %d", k);
						verbose_instrument(key_instr, true);
					}
					catch(...)
					{
						fputs("Illegal sub-instrument (an exception occured)", out_txt);
					}
				}
			}
			else	// Prevent instruments with multiple recursivities
				fputs("   Illegal double-recursive instrument !", out_txt);
			break;

		default :
			fputs("Unknown instrument type", out_txt);
			return;
	}
	if(recursive)
		fprintf(out_txt, "      Key : %d, Pan : %d\n", (inst.word1>>8) & 0xFF, inst.word1>>24);
}

static void parse_arguments(const int argc, char *const argv[])
{
	if (argc == 0) print_instructions();
	bool infile_found = false;
	bool outfile_found = false;

	for(int i = 0; i<argc; i++)
	{
		// Enable verbose if -v flag encountered in arguments list
		if(argv[i][0] == '-')
		{
			if(!strcmp(argv[i], "-v"))
			{
				verbose_flag = true;

				// Verbose to file if a file name is given
				if(i < argc-1 && argv[i+1][0] != '-')
				{
					verbose_output_to_file = true;
					out_txt = fopen(argv[i]+2, "w");
					if(!out_txt)
					{
						fprintf(stderr, "Invalid output text verbose file : %s\n", argv[i]+2);
						exit(-1);
					}
				}
			}

			// Change sampling rate if -s is encountered
			else if(argv[i][1] == 's')
			{
				change_sample_rate = true;
				sample_rate = atoi(argv[i]+2);
				if(!sample_rate)
				{
					fprintf(stderr, "Error, sampling rate %s is not a valid number.\n", argv[i]+2);
					exit(-1);
				}
			}
			
			// Change main volume if -mv is encountered
			else if(argv[i][1] == 'm' && argv[i][2] == 'v')
			{
				unsigned int volume = strtoul(argv[i]+3, 0, 10);
				if(volume==0 || volume>15)
				{
					fprintf(stderr, "Error, main volume %u is not valid (should be 0-15).\n", volume);
					exit(-1);
				}
				main_volume = volume;
			}
			else if(!strcmp(argv[i], "-gm"))
				gm_preset_names = true;
			
			else if(!strcmp(argv[i], "-help"))
				print_instructions();
		}

		// Try to parse an address and add it to list if succes
		else if(!infile_found)
		{
			// Input File
			infile_found = true;
			inGBA = fopen(argv[i], "rb");
			if(!inGBA)
			{
				fprintf(stderr, "Can't read input GBA file : %s\n", argv[0]);
				exit(-1);
			}
		}
		else if(!outfile_found)
		{
			outfile_found = true;
			size_t l = strlen(argv[i]);
			char *buffer = argv[i];
			if(l <= 4 || strcmp(argv[i] + (l-4), ".sf2"))
			{	// Append ".sf2" after the given file name if there isn't it already
				buffer = new char[l+4];
				strcpy(buffer, argv[i]);
				strcpy(buffer + l, ".sf2");
			}
			outSF2 = fopen(buffer, "wb");
			if(!outSF2)
			{
				fprintf(stderr, "Can't write on file : %s\n", argv[i]);
				exit(-1);
			}
			if(buffer != argv[i])
				delete[] buffer;
		}
		else
		{
			uint32_t address = strtoul(argv[i], 0, 0);
			if(!address) print_instructions();
			addresses.insert(address);				
		}
	}
	// Diagnostize errors/missing information
	if(!infile_found)
	{
		fputs("An input .gba file should be given. Use -help for more information.\n", stderr);
		exit(-1);
	}
	if(!outfile_found)
	{
		fputs("An output .sf2 file should be given. Use -help for more information.\n", stderr);
		exit(-1);
	}
	if(addresses.empty())
	{
		fputs("At least one adress should be given for decoding. Use -help for more information.\n", stderr);
		exit(-1);
	}
}

int main(const int argc, char *const argv[])
{
	puts("GBA ROM sound font ripper (c) 2012 Bregalad");

	// Parse arguments without the program name
	parse_arguments(argc-1, argv+1);

	// Compute prefix (path) of this program's name
	std::string prg_name = argv[0];
	std::string prg_prefix = prg_name.substr(0, prg_name.find("sound_font_riper"));

	// Create SF2 class
	sf2 = new SF2(sample_rate);
	instruments = new GBAInstr(sf2);

	// Attempt to access psg_data file
	psg_data = fopen((prg_prefix + "psg_data.raw").c_str(), "rb");
	if(!psg_data)
		puts("psg_data.raw file not found ! PSG Instruments can't be dumped.");

	// Attempt to access goldensun_synth file
	goldensun_synth = fopen((prg_prefix + "goldensun_synth.raw").c_str(), "rb");
	if(!goldensun_synth)
		puts("goldensun_synth.raw file not found ! Golden Sun's synth instruments can't be dumped.");

	// Read instrument data from input GBA file
	inst_data *instr_data = new inst_data[128];

	// Decode all banks
	current_bank = 0;
	for(std::set<uint32_t>::iterator it = addresses.begin(); it != addresses.end(); ++it, ++current_bank)
	{
		current_address = *it;
		std::set<uint32_t>::iterator next_it = it;
		++next_it;
		uint32_t next_address = *next_it;

		// Limit the # of presets if the addresses overlaps
		unsigned int ninstr = 128;
		if(addresses.end() != next_it && (next_address - current_address)/12 < 128)
			ninstr = (next_address - current_address)/12;

		// Seek at the start of the sound bank
		if(fseek(inGBA, current_address, SEEK_SET) != 0
		|| fread(instr_data, 4, ninstr*3, inGBA) != ninstr*3)				// Read entire sound bank in memory
		{
			fprintf(stderr, "Error : Invalid position within input GBA file : 0x%x\n", current_address);
			exit(0);
		}

		// Decode all instruments
		for(current_instrument = 0; current_instrument < ninstr; ++current_instrument, current_address += 12)
		{
			print("\nBank : " + std::to_string(current_bank) + ", Instrument : " + std::to_string(current_instrument) + " @0x" + hex(current_address));

			// Ignore unused instruments
			if(instr_data[current_instrument].word0 == 0x3c01
			&& instr_data[current_instrument].word1 == 0x02
			&& instr_data[current_instrument].word2 == 0x0F0000)
			{
				print(" (unused)");
				continue;
			}

			if(verbose_flag)
				verbose_instrument(instr_data[current_instrument], false);

			// Build equivalent SF2 instrument
			build_instrument(instr_data[current_instrument]);
		}
	}
	delete[] instr_data;

	if(verbose_output_to_file)
	{
		print("\n\n EOF");
		fclose(out_txt);
	}

	printf("\nDump complete, now outputting SF2 data...");

	sf2->write(outSF2);
	delete instruments;
	delete sf2;

	// Close files
	fclose(inGBA);

	if(psg_data) fclose(psg_data);
	if(goldensun_synth) fclose(goldensun_synth);

	puts(" Done !");
	return 0;
}
