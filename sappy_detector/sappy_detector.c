/**
 * GBA Sappy Engine Detector (c) 2012, 2014 by Bregalad
 * This is free and open source software
 * 
 * This programs detects if a snappy sound engine is present in a given GBA ROM.
 * If an engine is present it returns a pointer to the instrument list.
 * If no engine is present, it returns the value 0
 * It's not 100% accurate, so sometimes it might produce erroneous results
 * 
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <memory.h>

typedef const char *const string;

string sr_lookup[16] =
{
	"invalid", "5734 Hz", "7884 Hz", "10512 Hz", "13379 Hz", "15768 Hz", "18157 Hz",
	"21024 Hz", "26758 Hz", "31536 Hz", "36314 Hz", "40137 Hz", "42048 Hz", "invalid", "invalid", "invalid"
};

static void print_instructions()
{
	puts
	(
	   "GBA Sappy Engine Detector (c) 2015 by Bregalad and loveemu\n"
	   "Usage : sappy_detector game.gba\n"
	);
	exit(0);
}

static uint8_t m4a_bin_selectsong[0x1E] =
{
	0x00, 0xB5, 0x00, 0x04, 0x07, 0x4A, 0x08, 0x49, 
	0x40, 0x0B, 0x40, 0x18, 0x83, 0x88, 0x59, 0x00, 
	0xC9, 0x18, 0x89, 0x00, 0x89, 0x18, 0x0A, 0x68, 
	0x01, 0x68, 0x10, 0x1C, 0x00, 0xF0,
};

#define M4A_MAIN_PATT_COUNT 1
#define M4A_MAIN_LEN 2
static uint8_t m4a_bin_main[M4A_MAIN_PATT_COUNT][M4A_MAIN_LEN] =
{
	{0x00, 0xB5}
};

#define M4A_INIT_PATT_COUNT 2
#define M4A_INIT_LEN 2

// byte reader/writer (little-endian)
static inline uint32_t read_u32 (uint8_t *data) { return data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24); }

static long memsearch(uint8_t *dst, size_t dstsize, uint8_t *src, size_t srcsize, size_t dst_offset, size_t alignment, int diff_threshold)
{
	if (alignment == 0)
	{
		return -1;
	}

	// alignment
	if (dst_offset % alignment != 0)
	{
		dst_offset += alignment - (dst_offset % alignment);
	}

	for (size_t offset = dst_offset; (offset + srcsize) <= dstsize; offset += alignment)
	{
		// memcmp(&dst[offset], src, srcsize)
		int diff = 0;
		for (size_t i = 0; i < srcsize; i++)
		{
			if (dst[offset + i] != src[i])
			{
				diff++;
			}
			if (diff > diff_threshold)
			{
				break;
			}
		}
		if (diff <= diff_threshold)
		{
			return offset;
		}
	}
	return -1;
}

static bool is_valid_offset(uint32_t offset, uint32_t romsize)
{
	return (offset < romsize);
}

static bool is_gba_rom_address(uint32_t address)
{
	uint8_t region = (address >> 24) & 0xFE;
	return (region == 8);
}

static uint32_t gba_address_to_offset(uint32_t address)
{
	if (!is_gba_rom_address(address))
	{
		fprintf(stderr, "Warning: the address $%08X is not ROM address\n", address);
	}
	return address & 0x01FFFFFF;
}

/* Thanks to loveeemu for this routine, more accurate than mine ! Slightly adapted. */
#define M4A_OFFSET_SONGTABLE 40
static long m4a_searchblock(uint8_t *gbarom, size_t gbasize)
{
	long m4a_selectsong_offset = -1;
	long m4a_main_offset = -1;

	long m4a_selectsong_search_offset = 0;
	while (m4a_selectsong_search_offset != -1)
	{
		m4a_selectsong_offset = memsearch(gbarom, gbasize, m4a_bin_selectsong, sizeof(m4a_bin_selectsong), m4a_selectsong_search_offset, 1, 0);
		if (m4a_selectsong_offset != -1)
		{
#ifdef _DEBUG
			fprintf(stdout, "selectsong candidate: $%08X\n", m4a_selectsong_offset);
#endif

			// obtain song table address
			uint32_t m4a_songtable_address = read_u32(&gbarom[m4a_selectsong_offset + M4A_OFFSET_SONGTABLE]);
			if (!is_gba_rom_address(m4a_songtable_address))
			{
#ifdef _DEBUG
				fprintf(stdout, "Song table address error: not a ROM address $%08X\n", m4a_songtable_address);
#endif
				m4a_selectsong_search_offset = m4a_selectsong_offset + 1;
				continue;
			}
			uint32_t m4a_songtable_offset_tmp = gba_address_to_offset(m4a_songtable_address);
			if (!is_valid_offset(m4a_songtable_offset_tmp + 4 - 1, gbasize))
			{
#ifdef _DEBUG
				fprintf(stdout, "Song table address error: address out of range $%08X\n", m4a_songtable_address);
#endif
				m4a_selectsong_search_offset = m4a_selectsong_offset + 1;
				continue;
			}

			// song table must have more than one song
			int validsongcount = 0;
			for (int songindex = 0; validsongcount < 1; songindex++)
			{
				uint32_t songaddroffset = m4a_songtable_offset_tmp + (songindex * 8);
				if (!is_valid_offset(songaddroffset + 4 - 1, gbasize))
				{
					break;
				}

				uint32_t songaddr = read_u32(&gbarom[songaddroffset]);
				if (songaddr == 0)
				{
					continue;
				}

				if (!is_gba_rom_address(songaddr))
				{
#ifdef _DEBUG
					fprintf(stdout, "Song address error: not a ROM address $%08X\n", songaddr);
#endif
					break;
				}
				if (!is_valid_offset(gba_address_to_offset(songaddr) + 4 - 1, gbasize))
				{
#ifdef _DEBUG
					fprintf(stdout, "Song address error: address out of range $%08X\n", songaddr);
#endif
					break;
				}
				validsongcount++;
			}
			if (validsongcount < 1)
			{
				m4a_selectsong_search_offset = m4a_selectsong_offset + 1;
				continue;
			}
			break;
		}
		else
		{
			m4a_selectsong_search_offset = -1;
		}
	}
	if (m4a_selectsong_offset == -1)
	{
		return -1;
	}

	uint32_t m4a_main_offset_tmp = m4a_selectsong_offset;
	if (!is_valid_offset(m4a_main_offset_tmp + M4A_MAIN_LEN - 1, gbasize))
	{
		return -1;
	}
	while (m4a_main_offset_tmp > 0 && m4a_main_offset_tmp > ((uint32_t) m4a_selectsong_offset - 0x20))
	{
		for (int mainpattern = 0; mainpattern < M4A_MAIN_PATT_COUNT; mainpattern++)
		{
			if (memcmp(&gbarom[m4a_main_offset_tmp], &m4a_bin_main[mainpattern][0], M4A_INIT_LEN) == 0)
			{
				m4a_main_offset = (long) m4a_main_offset_tmp;
				break;
			}
		}
		m4a_main_offset_tmp--;
	}
	return m4a_main_offset;
}

typedef struct
{
	unsigned int polyphony           : 4;
	unsigned int main_vol            : 4;
	unsigned int sampling_rate_index : 4;
	unsigned int dac_bits            : 4;
}
sound_engine_param_t;

static sound_engine_param_t sound_engine_param(uint32_t data)
{
	sound_engine_param_t s;
	s.polyphony = (data & 0x000F00) >> 8;
	s.main_vol = (data & 0x00F000) >> 12;
	s.sampling_rate_index = (data & 0x0F0000) >> 16;
	s.dac_bits = 17-((data & 0xF00000) >> 20);
	return s;
}

// Test if an area of ROM is eligible to be the base pointer
static bool test_pointer_validity(uint32_t *data, uint32_t inGBA_length)
{
	sound_engine_param_t params = sound_engine_param(data[0]);

	/* Compute (supposed ?) address of song table */
	uint32_t song_tbl_adr = (data[2] & 0x3FFFFFF) + 12 * data[1];

	/* Prevent illegal values for all fields */
	return  params.main_vol != 0
	     && params.polyphony <= 12
	     && params.dac_bits >= 6
		 && params.dac_bits <= 9
	     && params.sampling_rate_index >= 1
	     && params.sampling_rate_index <= 12
	     && song_tbl_adr < inGBA_length
	     && data[1] < 256
	     &&((data[0] & 0xff000000) == 0);
}

int main(const int argc, string argv[])
{
	if(argc != 2) print_instructions();
	puts("Sappy sound engine detector (c) 2014 by Bregalad and loveemu\n");

	FILE *inGBA = fopen(argv[1], "rb");
	if(!inGBA)
	{
		fprintf(stderr, "Error : File %s can't be opened for reading.\n", argv[1]);
		exit(0);
	}

	/* Get the size of the input GBA file */
	fseek(inGBA, 0L, SEEK_END);
	const size_t inGBA_length = ftell(inGBA);

	uint8_t *inGBA_dump = (uint8_t*)malloc(inGBA_length);
	if(!inGBA_dump)
	{
		fprintf(stderr, "Error, can't allocate memory for ROM dump.\n");
		exit(0);
	}

	fseek(inGBA, 0L, SEEK_SET);
	size_t errcode = fread(inGBA_dump, 1, inGBA_length, inGBA);
	if(errcode != inGBA_length)
	{
		fprintf(stderr, "Error, can't dump ROM file. %x\n", errcode);
		exit(0);
	}
	fclose(inGBA);

	int32_t offset = m4a_searchblock(inGBA_dump, inGBA_length);
	
	if(offset < 0)
	{
		/* If no address were told manually and nothing was detected.... */
		puts("No sound engine was found.");
		exit(0);
	}
	printf("Sound engine detected at offset 0x%x\n", offset);

	/* Test validity of engine offset with -16 and -32 */
	bool valid_m16 = test_pointer_validity((uint32_t*)(inGBA_dump + offset - 16), inGBA_length);	// For most games
	bool valid_m32 = test_pointer_validity((uint32_t*)(inGBA_dump + offset - 32), inGBA_length);	// For pokémon

	/* If neither is found there is an error */
	if(!valid_m16 && !valid_m32)
	{
		puts("Only a partial sound engine was found.");
		exit(0);
	}
	offset -= valid_m16 ? 16 : 32;

	uint32_t *data = (uint32_t*)(inGBA_dump + offset);
	uint32_t song_tbl_adr = (data[2] & 0x3FFFFFF) + 12 * data[1];
	sound_engine_param_t params = sound_engine_param(data[0]);

	//Read # of song levels
	printf("# of song levels : %d\n", data[1]);

	// At this point we can be certain we detected the real thing.
	printf
	(
		"Engine parameters :\n"
		"Main Volume : %u Polyphony : %u channels, Dac : %u bits, Sampling rate : %s\n"
		"Song table located at : 0x%x\n",
		params.main_vol,
		params.polyphony,
		17-params.dac_bits,
		sr_lookup[params.sampling_rate_index],
		song_tbl_adr
	);
	
	free(inGBA_dump);

	/* Return the offset of sappy info to the operating system */
	return offset;
}