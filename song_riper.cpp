/**
 * GBA SongRiper (c) 2012, 2014 by Bregalad
 * This is free and open source software
 * 
 * This program convert a GBA song for the sappy sound engine into MIDI (.mid) format.
 */
 
#include "midi.hpp"
#include <algorithm>
#include <forward_list>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>

class Note;
// Global variables for this program

static uint32_t track_ptr[16];
static uint8_t last_cmd[16];
static char last_key[16];
static char last_vel[16];
static int counter[16];
static uint32_t return_ptr[16];
static int key_shift[16];
static bool return_flag[16];
static bool track_completed[16];
static bool end_flag = false;
static bool loop_flag = false;
static uint32_t loop_adr;

static int lfo_delay_ctr[16];
static int lfo_delay[16];
static int lfo_depth[16];
static int lfo_type[16];
static bool lfo_flag[16];
static bool lfo_hack[16];

static unsigned int simultaneous_notes_ctr = 0;
static unsigned int simultaneous_notes_max = 0;

static std::forward_list<Note> notes_playing;

static int bank_number;
static bool bank_used = false;
static bool rc = false;
static bool gs = false;
static bool xg = false;
static bool lv = false;
static bool sv = false;

static MIDI midi(24);
static FILE *inGBA;

static void process_event(int track);

static void print_instructions()
{
	puts(
		"Rips sequence data from a GBA game using sappy sound engine to MIDI (.mid) format.\n"
		"\nUsage : song_riper infile.gba outfile.mid song_adress [-b1 -gm -gs -xg]\n"
		"-b : Bank : forces all patches to be in the specified bank (0-127)\n"
		"In general MIDI, midi channel 10 is reserved for drums\n"
		"Unfortunately, we do not want to use any \"drums\" in the output file\n"
		"I have 3 modes to fix this problem\n"
		"-rc : Rearrange Channels. This will avoid using the channel 10, and use it at last ressort only if all 16 channels should be used\n"
		"-gs : This will send a GS system exclusive message to tell the player channel 10 is not drums\n"
		"-xg : This will send a XG system exclusive message, and force banks number which will disable \"drums\"\n"
		"-lv : Linearise volume and velocities. This should be used to have the output \"sound\" like the original song,\n"
		"      but shouldn't be used to get an exact dump of sequence data."
		"-sv : Simulate vibrato. This will insert controllers in real time to simulate a vibrato, instead of just\n"
		"      when commands are given. Like -lv, this should be used to have the output \"sound\" like the original song,\n"
		"      but shouldn't be used to get an exact dump of sequence data.\n"
		"It is possible, but not recommended, to use more than one of these flags at a time.\n"
	);
	exit(0);
}

static void add_simultaneous_note()
{
	// Update simultaneous notes max.
	if(++simultaneous_notes_ctr > simultaneous_notes_max)
		simultaneous_notes_max = simultaneous_notes_ctr;
}

// LFO logic on tick
static void process_lfo(int track)
{
	if(sv && lfo_delay_ctr[track] != 0)
	{
		// Decrease counter if it's value was nonzero
		if(--lfo_delay_ctr[track] == 0)
		{
			// If 1->0 transition we need to add a signal to start the LFO
			if(lfo_type[track] == 0)
				// Send a controller 1 if pitch LFO
				midi.add_controller(track, 1, (lfo_depth[track] < 16) ? lfo_depth[track] * 8 : 127);
			else
				// Send a channel aftertouch otherwise
				midi.add_chanaft(track, (lfo_depth[track] < 16) ? lfo_depth[track] * 8 : 127);
			lfo_flag[track] = true;
		}
	}
}

static void start_lfo(int track)
{
	// Reset down delay counter to its initial value
	if(sv && lfo_delay[track] != 0)
		lfo_delay_ctr[track] = lfo_delay[track];
}

static void stop_lfo(int track)
{
	// Cancel a LFO if it was playing,
	if(sv && lfo_flag[track])
	{
		if(lfo_type[track]==0)
			midi.add_controller(track, 1, 0);
		else
			midi.add_chanaft(track, 0);
		lfo_flag[track] = false;
	}
	else
		lfo_delay_ctr[track] = 0;			// cancel delay counter if it wasn't playing
}

// Note class
// this was needed to properly handle polyphony on all channels...

class Note
{
	MIDI& midi;
	int counter;
	int key;
	int vel;
	int chn;
	bool event_made;

	// Tick counter, if it becomes zero
	// then create key off event
	// this function returns "true" when the note should be freed from memory
	bool tick()
	{
		if(counter > 0 && --counter == 0)
		{
			midi.add_note_off(chn, key, vel);
			stop_lfo(chn);
			simultaneous_notes_ctr --;
			return true;
		}
		else
			return false;
	}
	friend bool countdown_is_over(Note& n);
	friend void make_note_on_event(Note& n);

public:
	// Create note and key on event
	Note(MIDI& midi, int chn, int len, int key, int vel) :
		midi(midi), counter(len), key(key), vel(vel), chn(chn)
	{
		event_made = false;

		start_lfo(chn);
		add_simultaneous_note();
	}
};

bool countdown_is_over(Note& n)
{
	return n.tick() || n.counter < 0;
}

void make_note_on_event(Note& n)
{
	if(!n.event_made)
	{
		midi.add_note_on(n.chn, n.key, n.vel);
		n.event_made = true;
	}
}

static bool tick(int track_amnt)
{
	// Tick all playing notes, and remove notes which
	// have been keyed off OR which are infinite length from the list
	notes_playing.remove_if(countdown_is_over);

	// Process all tracks
	for(int track = 0; track<track_amnt; track++)
	{
		counter[track]--;
		// Process events until counter non-null or pointer null
		// This might not be executed if counter both are non null.
		while(track_ptr[track] != 0 && !end_flag && counter[track] <= 0)
		{
			// Check if we're at loop start point
			if(track == 0 && loop_flag && !return_flag[0] && !track_completed[0] && track_ptr[0] == loop_adr)
				midi.add_marker("loopStart");

			process_event(track);
		}
	}

	for(int track = 0; track<track_amnt; track++)
	{
		process_lfo(track);
	}

	// Compute if all still active channels are completely decoded
	bool all_completed_flag = true;
	for(int i = 0; i < track_amnt; i++)
		all_completed_flag &= track_completed[i];

	// If everything is completed, the main program should quit its loop
	if(all_completed_flag) return false;

	// Make note on events for this tick
	//(it's important they are made after all other events)
	for_each(notes_playing.begin(), notes_playing.end(), make_note_on_event);

	// Increment MIDI time
	midi.clock();
	return true;
}

static uint32_t get_GBA_pointer()
{
	uint32_t p;
	fread(&p, 1, 4, inGBA);
	return p & 0x3FFFFFF;
}

static void process_event(int track)
{
	// Length table for notes and rests
	const int lenTbl[] =
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21, 22, 23, 24, 28, 30, 32, 36,
		40, 42, 44, 48, 52, 54, 56, 60, 64, 66, 68, 72, 76, 78,
		80, 84, 88, 90, 92, 96
	};

	fseek(inGBA, track_ptr[track], SEEK_SET);
	// Read command
	uint8_t command = fgetc(inGBA);

	track_ptr[track]++;
	uint8_t arg1;
	// Repeat last command, the byte read was in fact the first argument
	if(command < 0x80)
	{
		arg1 = command;
		command = last_cmd[track];
	}

	// Delta time command
	else if(command <= 0xb0)
	{
		counter[track] = lenTbl[command - 0x80];
		return;
	}

	// End track command
	else if(command == 0xb1)
	{
		// Null pointer
		track_ptr[track] = 0;
		track_completed[track] = true;
		return;
	}

	// Jump command
	else if(command == 0xb2)
	{
		track_ptr[track] = get_GBA_pointer();

		// detect the end track
		track_completed[track] = true;
		return;
	}

	// Call command
	else if(command == 0xb3)
	{
		uint32_t addr = get_GBA_pointer();

		// Return address for the track
		return_ptr[track] = track_ptr[track] + 4;
		// Now points to called address
		track_ptr[track] = addr;
		return_flag[track] = true;
		return;
	}

	// Return command
	else if(command == 0xb4)
	{
		if(return_flag[track])
		{
			track_ptr[track] = return_ptr[track];
			return_flag[track] = false;
		}
		return;
	}

	// Tempo change
	else if(command == 0xbb)
	{
		int tempo = 2*fgetc(inGBA);
		track_ptr[track]++;
		midi.add_tempo(tempo);
		return;
	}

	else
	{
		// Normal command
		last_cmd[track] = command;
		// Need argument
		arg1 = fgetc(inGBA);
		track_ptr[track]++;
	}

	// Note on with specified length command
	if(command >= 0xd0)
	{
		int key, vel, len_ofs = 0;
		// Is arg1 a key value ?
		if(arg1 < 0x80)
		{	// Yes -> use new key value
			key = arg1;
			last_key[track] = key;

			uint8_t arg2 = fgetc(inGBA);
			// Is arg2 a velocity ?
			if(arg2 < 0x80)
			{	// Yes -> use new velocity value
				vel = arg2;
				last_vel[track] = vel;
				track_ptr[track]++;

				uint8_t arg3 = fgetc(inGBA);

				// Is there a length offset ?
				if(arg3 < 0x80)
				{	// Yes -> read it and increment pointer
					len_ofs = arg3;
					track_ptr[track]++;
				}
			}
			else
			{	// No -> use previous velocity value
				vel = last_vel[track];
			}
		}
		else
		{
			// No -> use last value
			key = last_key[track];
			vel = last_vel[track];
			track_ptr[track]--;		// Seek back, as arg 1 is unused and belong to next event !
		}

		// Linearise velocity if needed
		if(lv) vel = sqrt(127.0 * vel);

		notes_playing.push_front( Note(midi, track, lenTbl[command - 0xd0 + 1] + len_ofs, key + key_shift[track], vel) );
		return;
	}

	// Other commands
	switch(command)
	{
		// Key shift
		case 0xbc :
			key_shift[track] = arg1;
			return;

		// Set instrument
		case 0xbd :
			if(bank_used)
			{
				if(!xg)
					midi.add_controller(track, 0, bank_number);
				else
				{
					midi.add_controller(track, 0, bank_number >> 7);
					midi.add_controller(track, 32, bank_number & 0x7f);
				}
			}
			midi.add_pchange(track, arg1);
			return;

		// Set volume
		case 0xbe :
		{	// Linearise volume if needed
			int volume = lv ? (int)sqrt(127.0 * arg1) : arg1;
			midi.add_controller(track, 7, volume);
		}	return;

		// Set panning
		case 0xbf :
			midi.add_controller(track, 10, arg1);
			return;

		// Pitch bend
		case 0xc0 :
			midi.add_pitch_bend(track, (char)arg1);
			return;

		// Pitch bend range
		case 0xc1 :
			if(sv)
				midi.add_RPN(track, 0, (char)arg1);
			else
				midi.add_controller(track, 20, arg1);
			return;

		// LFO Speed
		case 0xc2 :
			if(sv)
				midi.add_NRPN(track, 136, (char)arg1);
			else
				midi.add_controller(track, 21, arg1);
			return;

		// LFO delay
		case 0xc3 :
			if(sv)
				lfo_delay[track] = arg1;
			else
				midi.add_controller(track, 26, arg1);
			return;

		// LFO depth
		case 0xc4 :
			if(sv)
			{
				if(lfo_delay[track] == 0 && lfo_hack[track])
				{
					if(lfo_type[track]==0)
						midi.add_controller(track, 1, arg1>12 ? 127 : 10 * arg1);
					else
						midi.add_chanaft(track, arg1>12 ? 127 : 10 * arg1);

					lfo_flag[track] = true;
				}
				lfo_depth[track] = arg1;
				// I had a stupid bug with LFO inserting controllers I didn't want at the start of files
				// So I made a terrible quick fix for it, in the mean time I can find something better to prevent it.
				lfo_hack[track] = true;
			}
			else
				midi.add_controller(track, 1, arg1);
			return;

		// LFO type
		case 0xc5 :
			if(sv)
				lfo_type[track] = arg1;
			else
				midi.add_controller(track, 22, arg1);
			return;

		// Detune
		case 0xc8 :
			if(sv)
				midi.add_RPN(track, 1, (char)arg1);
			else
				midi.add_controller(track, 24, arg1);
			return;

		// Key off
		case 0xce :
		{
			int key, vel = 0;

			// Is arg1 a key value ?
			if(arg1 < 0x80)
			{	// Yes -> use new key value
				key = arg1;
				last_key[track] = key;
			}
			else
			{	// No -> use last value
				key = last_key[track];
				vel = last_vel[track];
				track_ptr[track]--;		// Seek back, as arg 1 is unused and belong to next event !
			}

			midi.add_note_off(track, key + key_shift[track], vel);
			stop_lfo(track);
			simultaneous_notes_ctr --;
		}	return;

		// Key on
		case 0xcf :
		{
			int key, vel;
			// Is arg1 a key value ?
			if(arg1 < 0x80)
			{
				// Yes -> use new key value
				key = arg1;
				last_key[track] = key;

				uint8_t arg2 = fgetc(inGBA);
				// Is arg2 a velocity ?
				if(arg2 < 0x80)
				{
					// Yes -> use new velocity value
					vel = arg2;
					last_vel[track] = vel;
					track_ptr[track]++;					
				}
				else	// No -> use previous velocity value
					vel = last_vel[track];
			}
			else
			{
				// No -> use last value
				key = last_key[track];
				vel = last_vel[track];
				track_ptr[track]--;		// Seek back, as arg 1 is unused and belong to next event !
			}
			// Linearise velocity if needed
			if(lv) vel = (int)sqrt(127.0 * vel);

			// Make note of infinite length
			notes_playing.push_front(Note(midi, track, -1, key + key_shift[track], vel));
		}	return;

		default :
			break;
	}
}

static uint32_t parseArguments(const int argv, const char *const args[])
{
	if (argv < 3) print_instructions();

	// Open the input and output files
	inGBA = fopen(args[0], "rb");
	if(!inGBA)
	{
		fprintf(stderr, "Can't open file %s for reading.\n", args[0]);
		exit(0);
	}

	for(int i=3; i<argv; i++)
	{
		if(args[i][0] == '-')
		{
			if(args[i][1] == 'b')
			{
				if(strlen(args[i]) <3) print_instructions();
				bank_number = atoi(args[i]+2);
				bank_used = true;
			}
			else if(args[i][1]=='r' && args[i][2]=='c')
				rc = true;
			else if(args[i][1]=='g' && args[i][2]=='s')
				gs = true;
			else if(args[i][1]=='x' && args[i][2]=='g')
				xg = true;
			else if(args[i][1]=='l' && args[i][2]=='v')
				lv = true;
			else if(args[i][1]=='s' && args[i][2]=='v')
				sv = true;
			else
				print_instructions();
		}
		else
			print_instructions();
	}
	// Return base adress, parsed correctly in both decimal and hex
	return strtoul(args[2], 0, 0);
}

int main(int argc, char *argv[])
{
	FILE *outMID;
	puts("GBA ROM sequence ripper (c) 2012 Bregalad");
	uint32_t base_address = parseArguments(argc-1, argv+1);

	if(fseek(inGBA, base_address, SEEK_SET))
	{
		fprintf(stderr, "Can't seek to the base adress 0x%x.\n", base_address);
		exit(0);
	}

	int track_amnt = fgetc(inGBA);
	if(track_amnt < 1 || track_amnt > 16)
	{
		fprintf(stderr, "Invalid amount of tracks %d ! (must be 1-16)\n", track_amnt);
		exit(0);
	}
	printf("%u tracks.\n", track_amnt);

	// Open output file once we know the pointer points to correct data
	//(this avoids creating blank files when there is an error)
	outMID = fopen(argv[2], "wb");
	if(!outMID)
	{
		fprintf(stderr, "Can't write on file %s\n", argv[2]);
		exit(0);
	}

	printf("Converting...");

	if(rc)
	{	// Make the drum channel last in the list, hopefully reducing the risk of it being used
		midi.chn_reorder[9] = 15;
		for(unsigned int j=10; j < 16; ++j)
			midi.chn_reorder[j] = j-1;
	}

	if(gs)
	{	// GS reset
		const char gs_reset_sysex[] = {0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41};
		midi.add_sysex(gs_reset_sysex, sizeof(gs_reset_sysex));
		// Part 10 to normal
		const char part_10_normal_sysex[] = {0x41, 0x10, 0x42, 0x12, 0x40, 0x10, 0x15, 0x00, 0x1b};
		midi.add_sysex(part_10_normal_sysex, sizeof(part_10_normal_sysex));
	}

	if(xg)
	{	// XG reset
		const char xg_sysex[] = {0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00};
		midi.add_sysex(xg_sysex, sizeof xg_sysex);
	}

	midi.add_marker("Converted by SequenceRipper 2.0");

	fgetc(inGBA);						// Unknown byte
	fgetc(inGBA);						// Priority
	int8_t reverb = fgetc(inGBA);		// Reverb

	int instr_bank_address = get_GBA_pointer();

	// Read table of pointers
	for(int i=0; i<track_amnt; i++)
	{
		track_ptr[i] = get_GBA_pointer();

		lfo_depth[i] = 0;
		lfo_delay[i] = 0;
		lfo_flag[i] = false;

		if(reverb < 0)  // add reverb controller on all tracks
			midi.add_controller(i, 91, lv ? (int)sqrt((reverb&0x7f)*127.0) : reverb&0x7f);
	}

	// Search for loop address of track #0
	if(track_amnt > 1)	// If 2 or more track, end of track is before start of track 2
		fseek(inGBA, track_ptr[1] - 9, SEEK_SET);
	else
		// If only a single track, the end is before start of header data
		fseek(inGBA, base_address - 9, SEEK_SET);

	// Read where in track 1 the loop starts
	for(int i=0; i<5; i++)
		if(fgetc(inGBA) == 0xb2)
		{
			loop_flag = true;
			loop_adr = get_GBA_pointer();
			break;
		}

	// This is the main loop which will process all channels
	// until they are all inactive
	int i = 100000;
	while(tick(track_amnt))
	{
		if(i-- == 0)
		{	// Security thing to avoid infinite loop in case things goes wrong
			puts("Time out!");
			break;
		}
	}

	// If a loop was detected this is its end
	if(loop_flag) midi.add_marker("loopEnd");

	printf("Maximum simultaneous notes : %d\n", simultaneous_notes_max);

	printf("Dump complete. Now outputting MIDI file...");
	midi.write(outMID);
	// Close files
	fclose(inGBA);
	puts(" Done !");
	return instr_bank_address;
}