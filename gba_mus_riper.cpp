/**
 * GBAMusRiper (c) 2012 by Bregalad
 * This is free and open source software
 * 
 * This program analyze a Game Boy Advance ROM and search for a sound engine
 * named "Sapy" which is used in ~90% of commercial GBA games.
 * 
 * If the engine is found it rips all musics to MIDI (.mid) format and all
 * instruments to SoundFont 2.0 (.sf2) format.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <set>
#include "hex_string.hpp"

#ifndef WIN32
namespace sappy_detector
{
	#include "sappy_detector.c"		// The main::function is called directly on linux
}
#endif

static FILE *inGBA;
static std::string inGBA_path;
static size_t inGBA_size;
static std::string name;
static std::string path;
static bool gm = false;
static bool xg = false;
static bool rc = false;
static bool sb = false;
static bool raw = false;
static uint32_t song_tbl_ptr = 0;

static const int sample_rates[] = {-1, 5734, 7884, 10512, 13379, 15768, 18157, 21024, 26758, 31536, 36314, 40137, 42048};

static void print_instructions()
{
	puts(
		"  /==========================================\\\n"
		"-<   GBA Mus Riper 3.1 (c) 2015 by Bregalad   >-\n"
		"  \\==========================================/\n\n"
		"Usage : gba_mus_riper game.gba [address] [flags]\n\n"
		"-gm  : Give General MIDI names to presets. Note that this will only change the names and will NOT\n"
		"       magically turn the soundfont into a General MIDI compliant soundfont.\n"
		"-rc  : Rearrange channels in output MIDIs so channel 10 is avoided. Needed by sound\n"
		"-xg  : Output MIDI will be compliant to XG standard (instead of default GS standard)\n"
		"       cards where it's impossible to disable \"drums\" on channel 10 even with GS or XG commands\n"
		"-sb  : Separate banks. Every sound bank is riper to a different .sf2 file and placed\n"
		"       into different sub-folders (instead of doing it in a single .sf2 file and a single folder)\n"
		"-raw : Output MIDIs exactly as they're encoded in ROM, without linearise volume and\n"
		"       velocities and without simulating vibratos.\n"
		"-adr : Force adress of the song table manually. This is required for manually dumping music data\n"
		"       from ROMs where the location can't be detected automatically.\n"
	);
	exit(0);
}

static uint32_t get_GBA_pointer()
{
	uint32_t p;
	fread(&p, 4, 1, inGBA);
	return p - 0x8000000;
}

static void mkdir(std::string name)
{
	system(("mkdir \"" + name + '"').c_str());
}

//  Convert number to string with always 3 digits (even if leading zeroes)
static std::string dec3(unsigned int n)
{
	std::string s;
	s += "0123456789"[n/100];
	s += "0123456789"[n/10%10];
	s += "0123456789"[n%10];
	return s;
}

static void parse_args(const int argc, char *const args[])
{
	if(argc < 1) print_instructions();

	bool path_found = false, song_tbl_found = false;
	for(int i = 0; i<argc; i++)
	{
		if(args[i][0] == '-')
		{
			if(!strcmp(args[i], "-help"))
				print_instructions();
			else if(!strcmp(args[i], "-gm"))
				gm = true;
			else if(!strcmp(args[i], "-xg"))
				xg = true;
			else if(!strcmp(args[i], "-rc"))
				rc = true;
			else if(!strcmp(args[i], "-sb"))
				sb = true;
			else if(!strcmp(args[i], "-raw"))
				raw = true;
			else
			{
				fprintf(stderr, "Error : Unknown command line option : %s. Try with -help to get information.\n", args[i]);
				exit(-1);
			}
		}
		// Convert given address to binary, use it instead of automatically detected one
		else if(!path_found)
		{
			// Get GBA file
			inGBA = fopen(args[i], "rb");
			if(!inGBA)
			{
				fprintf(stderr, "Error : Can't open file %s for reading.\n", args[i]);
				exit(-1);
			}

			// Name is filename without the extention and without path
			inGBA_path = std::string(args[i]);
			size_t separator_index = inGBA_path.find_last_of("/\\") + 1;
			name = inGBA_path.substr(separator_index, inGBA_path.find_last_of('.') - separator_index);

			// Path where the input GBA file is located
			path = inGBA_path.substr(0, separator_index);
			path_found = true;
		}
		else if(!song_tbl_found)
		{
			errno = 0;
			song_tbl_ptr = strtoul(args[i], 0, 0);
			if(errno)
			{
				fprintf(stderr, "Error : %s is not a valid song table address.\n", args[i]);
				exit(-1);
			}
			song_tbl_found = true;
		}
		else
		{
			fprintf(stderr, "Error : Don't know what to do with %s. Try with -help to get more information.\n", args[i]);
			exit(-1);
		}
	}
	if(!path_found)
	{
		fputs("Error : No input GBA file. Try with -help to get more information.\n", stderr);
		exit(-1);
	}
}

int main(int argc, char *const argv[])
{
	// Parse arguments (without program name)
	parse_args(argc-1, argv+1);

	// Compute program prefix (should be "", "./", "../" or whathever)
	std::string prg_name = argv[0];
	std::string prg_prefix = prg_name.substr(0, prg_name.rfind("gba_mus_riper"));

	int sample_rate = 0, main_volume = 0;		// Use default values when those are '0'

	// If the user hasn't provided an address manually, we'll try to automatically detect it
	if(!song_tbl_ptr)
	{
		// Auto-detect address of sappy engine
#ifdef WIN32
		// On windows, just use the 32-bit return code of the sappy_detector executable
		std::string sappy_detector_cmd = prg_prefix + "sappy_detector \"" + inGBA_path + '"';
		int sound_engine_adr = std::system(sappy_detector_cmd.c_str());
#else
		// On linux the function is duplicated in this executable
		const char *sappy_detector_argv1 = inGBA_path.c_str();
		int sound_engine_adr = sappy_detector::main(2, &sappy_detector_argv1 - 1);
#endif

		// Exit if no sappy engine was found
		if(!sound_engine_adr) exit(0);

		if(fseek(inGBA, sound_engine_adr, SEEK_SET))
		{
			fprintf(stderr, "Error : Invalid offset within input GBA file : 0x%x\n", sound_engine_adr);
			exit(0);
		}

		// Engine parameter's word
		uint32_t parameter_word;
		fread(&parameter_word, 4, 1, inGBA);

		// Get sampling rate
		sample_rate = sample_rates[(parameter_word>>16) & 0xf];
		main_volume = (parameter_word>>12) & 0xf;

		// Compute address of song table
		uint32_t song_levels;			// Read # of song levels
		fread(&song_levels, 4, 1, inGBA);
		printf("# of song levels : %d\n", song_levels);
		song_tbl_ptr = get_GBA_pointer() + 12 * song_levels;
	}

	// Create a directory named like the input ROM, without the .gba extention
	mkdir(name);

	//  Get the size of the input GBA file
	fseek(inGBA, 0L, SEEK_END);
	inGBA_size = ftell(inGBA);
	
	if(song_tbl_ptr >= inGBA_size)
	{
		fprintf(stderr, "Fatal error : Song table at 0x%x is past the end of the file.\n", song_tbl_ptr);
		exit(0);
	}

	printf("Parsing song table...");
	// New list of songs
	std::vector<uint32_t> song_list;
	// New list of sound banks
	std::set<uint32_t> sound_bank_list;

	if(fseek(inGBA, song_tbl_ptr, SEEK_SET))
	{
		fprintf(stderr, "Fatal error : Can't seek to song table at : 0x%x\n", song_tbl_ptr);
		exit(0);
	}

	// Ignores entries which are made of 0s at the start of the song table
	// this fix was necessarily for the game Fire Emblem
	uint32_t song_pointer;
	while(true)
	{
		fread(&song_pointer, 4, 1, inGBA);
		if(song_pointer != 0) break;
		song_tbl_ptr += 4;
	}

	unsigned int i = 0;
	while(true)
	{
		song_pointer -= 0x8000000;		// Adjust pointer

		// Stop as soon as we met with an invalid pointer
		if(song_pointer == 0 || song_pointer >= inGBA_size) break;

		for(int j=4; j!=0; --j) fgetc(inGBA);		// Discard 4 bytes (sound group)
		song_list.push_back(song_pointer);			// Add pointer to list
		i++;
		fread(&song_pointer, 4, 1, inGBA);
	};
	// As soon as data that is not a valid pointer is found, the song table is terminated

	// End of song table
	uint32_t song_tbl_end_ptr = 8*i + song_tbl_ptr;

	puts("Collecting sound bank list...");

	typedef std::set<uint32_t>::iterator bank_t;
	bank_t *sound_bank_index_list = new bank_t[song_list.size()];

	for(i = 0; i < song_list.size(); i++)
	{
		// Ignore unused song, which points to the end of the song table (for some reason)
		if(song_list[i] != song_tbl_end_ptr)
		{
			// Seek to song data
			if(fseek(inGBA, song_list[i] + 4, SEEK_SET)) continue;
			uint32_t sound_bank_ptr = get_GBA_pointer();

			// Add sound bank to list if not already in the list
			sound_bank_index_list[i] = sound_bank_list.insert(sound_bank_ptr).first;
		}
	}

	// Close GBA file so that SongRiper can access it
	fclose(inGBA);
	
	// Create directories for each sound bank if separate banks is enabled
	if(sb)
	{
		for(bank_t j = sound_bank_list.begin(); j != sound_bank_list.end(); ++j)
		{
			unsigned int d = std::distance(sound_bank_list.begin(), j);
			std::string subdir = name + '/' + "soundbank_" + dec3(d);
			mkdir(subdir);
		}
	}

	for(i=0; i < song_list.size(); i++)
	{
		if(song_list[i] != song_tbl_end_ptr)
		{
			unsigned int bank_index = distance(sound_bank_list.begin(), sound_bank_index_list[i]);
			std::string seq_rip_cmd = prg_prefix + "song_riper \"" + inGBA_path + "\" \"" + name;

			// Add leading zeroes to file name
			if(sb) seq_rip_cmd += "/soundbank_" + dec3(bank_index);
			seq_rip_cmd += "/song" + dec3(i) + ".mid\"";

			seq_rip_cmd += " 0x" + hex(song_list[i]);
			seq_rip_cmd += rc ? " -rc" : (xg ? " -xg" : " -gs");
			if(!raw)
			{
				seq_rip_cmd += " -sv";
				seq_rip_cmd += " -lv";
			}
			// Bank number, if banks are not separated
			if(!sb)
				seq_rip_cmd += " -b" + std::to_string(bank_index);

			printf("Song %u\n", i);
			
// 			printf("DEBUG : Going to call system(%s)\n", seq_rip_cmd.c_str());
			if(!system(seq_rip_cmd.c_str())) puts("An error has occured.");
		}
	}
	delete[] sound_bank_index_list;

	if(sb)
	{
		// Rips each sound bank in a different file/folder
		for(bank_t j=sound_bank_list.begin(); j!=sound_bank_list.end(); ++j)
		{
			unsigned int bank_index = distance(sound_bank_list.begin(), j);

			std::string sbnumber = dec3(bank_index);
			std::string foldername = "soundbank_" + sbnumber;
			std::string sf_rip_args = prg_prefix + "sound_font_riper \"" + inGBA_path + "\" \"" + name + '/';
			sf_rip_args += foldername + '/' + foldername /* + "_@" + hex(*j) */ + ".sf2\"";

			if(sample_rate) sf_rip_args += " -s" + std::to_string(sample_rate);
			if(main_volume)	sf_rip_args += " -mv" + std::to_string(main_volume);
			if(gm) sf_rip_args += " -gm";
			sf_rip_args += " 0x" + hex(*j);

// 			printf("DEBUG : Goint to call system(%s)\n", sf_rip_args.c_str());
			system(sf_rip_args.c_str());
		}
	}
	else
	{
		// Rips each sound bank in a single soundfont file
		// Build argument list to call sound_font_riper
		// Output sound font named after the input ROM
		std::string sf_rip_args = prg_prefix + "sound_font_riper \"" + inGBA_path + "\" \"" + name + '/' + name + ".sf2\"";
		if(sample_rate) sf_rip_args += " -s" + std::to_string(sample_rate);
		if(main_volume) sf_rip_args += " -mv" + std::to_string(main_volume);
		// Pass -gm argument if necessary
		if(gm) sf_rip_args += " -gm";

		// Make sound banks addresses list.
		for(bank_t j=sound_bank_list.begin(); j != sound_bank_list.end(); ++j)
			sf_rip_args += " 0x" + hex(*j);

		// Call sound font riper
// 		printf("DEBUG : Going to call system(%s)\n", sf_rip_args.c_str());
		system(sf_rip_args.c_str());
	}
	puts("Rip completed !");
	return 0;
}
