=======================================
GBA Mus Riper by Bregalad
Release 2.4; 3rd April 2014
=======================================

GBA Mus Riper is a suite of programs to rip music from Game Boy Advance (GBA) games using the "Sappy" sound engine, a really common engine among commercial GBA games.
Music rippers from GBA games already exists, but this one is more complete and versatile, and less buggy than the existing ones (as for the date of writing this).
It's the first one that actually converts instruments to SoundFont format, including GameBoy instruments.

GBAMusRiper is free and open source software. Anyone is free to redistribute it (with and without sources) and to improve it but just give me credit, THANK YOU VERY MUCH.

Applications :
- Listen to GBA music with higher quality sound hardware than a GBA or an emulated GBA which only have 8-bit sound
- Make other music sound like a GBA game.
- Simply get music sheets of GBA games.

The tools comes in the form of 4 separate executable files.

IMPORTANT NOTE : The "gba_mus_riper" executable simply calls the other 3 executables in the process of ripping the music, so even if you're among the 99% of users that will only use "gba_mus_riper.exe" and never touch the other 3 executables, do NOT remove, move or rename them, because "gba_mus_riper.exe" would also stop working !

== 1) GBA Mus Riper ==

This is the main program which does everything automatically. Normally you'd only want to use this one, but the other 3 sub-programs can also be used individually too.
This program detects the presence of the sappy sound engine in a given ROM, and converts all detected songs to .mid files, and rip all detected sound banks to a .sf2 file.

!!! HOW TO USE THIS PROGRAM !!!

Since v2.0 Java is not required any longer. On Windows you should just open a console prompt in the folder containing the ".exe" files, and type gba_mus_riper followed by the path name of the GBA rom you'd wan to rip sounds from (hint : you can drag and drop the ROM into the command line).
On Linux you should compile the program yourself, see compilation notes a few chapters below. Once the program is compiled into executable form, it's usage is identical.

The following options are available :

-gm : Give General MIDI names to presets. Note that this will only change the names and will NOT
      magically turn the soundfont into a General MIDI compliant soundfont.
-xg : Output MIDI will be compliant to XG standard (instead of default GS standard)
-rc : Rearrange channels in output MIDIs so channel 10 is avoided. Needed by sound
      cards where it's impossible to disable "drums" on channel 10 even with GS or XG commands
-sb : Separate banks. Every sound bank is riper to a different .sf2 file and placed
      into different sub-folders (instead of doing it in a single .sf2 file and a single folder)
-raw : Output MIDIs exactly as they're encoded in ROM, without linearise volume and
       velocities and without simulating vibratos.

== 2) Sappy Detector ==

This program is here to detect the sappy sound engine. If an engine is found, it prints info about how the game uses the engine on the screen. This is the easiest way to know if a given GBA games use the sappy sound engine or not.

Usage : sappy_detector game.gba


== 3) Song Riper ==

This program rips sequence data from a GBA game using sappy sound engine to MIDI (.mid) format. You'd typically use this if you'd like to get the spreadsheet of a particular song with more options available than a plain dump with the default parameters from GBA Mus Riper.

Usage :
song_riper infile.gba outfile.mid song_adress [flags]

-b[n] : Bank : forces all patches to be in the specified bank (0-127)
-gm : Give General MIDI names to presets. Note that this will only change the names and will NOT magically turn the soundfont into a General MIDI compliant soundfont.
In general MIDI, midi channel 10 is reserved for drums
Unfortunately, we do not want to use any "drums" in the output file
I have 3 modes to fix this problem
-rc : Rearrange Channels. This will avoid using the channel 10, and use it at last ressort only if all 16 channels should be used
-gs : This will send a GS system exclusive message to tell the player channel 10 is not "drums"
-xg : This will send a XG system exclusive message, and force banks number which will disable "drums"

-lv : Linearise volume and velocities. This should be used to have the output "sound" like the original song, but shouldn't be used to get an exact dump of sequence data.
-sv : Simulate vibrato. This will insert controllers in real time to simulate a vibrato, instead of just when commands are given. Like -lv, this should be used to have the output "sound" like the original song,but shouldn't be used to get an exact dump of sequence data.

== 4) Sound Font Riper ==

Dumps a sound bank (or a list of sound banks) from a GBA game which is using the sappy sound engine to SoundFont 2.0 (.sf2) format. You'd typically use this to get a SoundFont dump of data within a GBA game directly without dumping any other data.

Usage :
sound_font_riper in.gba out.sf2 [flags] address1 [address2] ....

Instruments at address1 will be dumped to Bank0, instruments at address2 to Bank1, etc.....

Flags :
-v : verbose : Display info about the sound font in text format. If -v is followed directly by a file name (as in -vmyfile.txt), info is printed to the specified file instead.
-s : Sampling rate for samples. Default : 22050 Hz
-gm : Give General MIDI names to presets. Note that this will only change the names and will NOT magically turn the soundfont into a General MIDI compliant soundfont.
-mv : Main volume for sample instruments. Range : 1-15. Game Boy channels are unaffected.

IMPORTANT NOTE : You need to leave the included file "psg_data.raw" and "goldensun_synth.raw" INTACT for Sound Font Riper to work properly. If you remove or affect the files in any way, the "old" Game Boy PSG instruments and the Godlen Sun's synth instrument (respecively) won't be dumped at all.

== HOWTO : Playback converted MIDIs ==

To play back MIDIs I recommend Winamp or Foobar2000. Unfortunately it seems song looping is error prone in Winamp (Nullsoft MIDI reader v3.5). I hope they'll fix it some day.
To use SoundFont data to play the ripped MIDIs, I recommend BASSMIDI driver, which is free and allows playback of MIDI files using a sound font file.

Both MIDI and SF2 are widely used standards so there is a wide range of programs supporting these.

== HOWTO : Compile the GBA Mus Riper suite ==

First you should edit the Makefile (don't worry it's a very simple one) to suit your needs (compiler, flags, etc...). You need support for C++11, this means if you're using gcc you're going to need a version more recent than 4.8. It's probably compilable on 4.7.x but it's simpler to just update the compiler.

Also if you insist on using something else than gcc, you should be very careful as somewhere in sf2_chunks.h, there is a struct class that must be packed in order to output correct data. If your compiler doesn't support the non-standard __attribute__ ((packed)) extension you'd have to figure out another way around the problem by yourself.
One of the files is .c instead of .cpp but this file is compatible with both C99 and C++11 really, it just doesn't use any of the C++ extensions.

== HOWTO : Rip songs semi-manually ==

When the automatic detection fails, it's possible (and actually fairly easy) to rip the data in a semi-automatic way. Semi-automatic because you have to locate the song table yourself within the ROM, but then the songs and their sound fonts are still dumped automatically by GBA Mus Riper (you don't have to call Song Riper and Sound Font Riper manually, although you could of course do this).

To locate the song table within the ROM is pretty simple, but you need an HEX editor with search. Search for the sequence : 0x01, 0x3c, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00
Congratulations, you just found an "unused instrument". If there is several of them next to each other then there is not a single doubt the GBA ROM uses sappy (despite it's header not being detected).
Once you found this parameter your hex editor so that it shows a multiple of 8 bytes per row (8, 16 or 24), and scroll down until you see "colomns" of 0x00 and 0x08 appearing. It should look like this :
0x** 0x** 0x** 0x08 0x0* 0x00 0x0* 0x00 0x** 0x** 0x** 0x08 0x0* 0x00 0x0* 0x00
0x** 0x** 0x** 0x08 0x0* 0x00 0x0* 0x00 0x** 0x** 0x** 0x08 0x0* 0x00 0x0* 0x00
0x** 0x** 0x** 0x08 0x0* 0x00 0x0* 0x00 0x** 0x** 0x** 0x08 0x0* 0x00 0x0* 0x00

Now just find the location of the very first word of 4 bytes that ends in 0x08 (it should be 32-bit aligned), and congratulations, you found the pointer to the first song of the game. For example if the location was 0xabcde0 you should use :
gba_mus_riper game.gba 0xabcde0

It's not that complicated, is it ?
Now if this still too compilcated there is yet another alternative. Search for a tool called "mp2ktool", which can locate this automatically (and more accurately than my own Sappy Detector). So just use :
mp2ktool songtable game.gba
and it will print the offset on the screen, then use this offset as an argument to gba_mus_riper and it'll rip it.
I only heard about this tool very recently but it sounds interesting I'd have to give it a deeper investigation.

== Note about the old Java version 1.1 ==

The version 1.1 of the program, written in Java and requiring the JRE to run, is still provided as archive for fallback for people who can't run nor compile the new version. This version is not supported by any means. It's usage and functionality is roughly equivalent to v2.0 with less features.

== FREQUENTLY ASKED QUESTIONS ==

Q : How do I know if game XXX uses sappy engine ?
A : Just try to run sappy_detector on it, if it doesn't work then it's very likely the game doesn't use the sappy engine. A good 80-90% of commercial GBA games uses this engine.

Q : If the sappy detector doesn't detect the engine header, does this mean the game doesn't use the sappy engine ?
A : No. The detector is heuristic. There is games that uses sappy and that have no header. Metroid games comes in mind. Since v2.0 it's still possible to rip them by manually telling gba_mus_riper the song table address. It will just not know the base sample rate and main volume, and use default values for them. This could affect the audibility of the music significantly.
On the other side, there might be fake positives, that is cases where the detector "thinks" there is a sappy header when there is none. If that's the case the program will start to jump garbage, but will probably encounter an error before the thing terminates. If this happens, you'd also have to give the address of the song table manually.

Q : Will it sound EXACTLY like on the GBA ?
A : No, but in all honesty very damn close. The sampled instruments will actually sound BETTER, because the GBA sound hardware has 8-bit sound output, this means the Digital to Analog in the GBA can only output 256 voltage levels, which means the output is being quantified and is very noisy.
On the other side the good old GameBoy instruments will sound different because the're emulated using samples. These instruments are quantified in volume and pitch, and there is no way to reproduce this imperfection in the .SF2 standard. I used samples actually recorded from my DS in GBA mode.
The Golden Sun synth instrument, added in version 2.2, also does not sound exactly like their original counterpart, again because they're emulated using samples. The assembly code for those instruments was unrolled and extremely optimised, therefore it was very hard to know exactly how those instruments were designed.

Q : Can I re-use the soundfont to play other MIDIs / Can I use soundfont from Game A to play music from Game B ?
A : Yes but unfortunately there is a 99% of chances it will sound like total crap, because very few games have a Gerneral MIDI compliant soundfont. If you're looking for games with partially Genral Midi compliant soundfonts, I know of Golden Sun, my Final Fantasy V and VI hacks, and Castlevania : Aria of Sorrow. (All of these have a lot of missing instruments and some non-General MIDI compilant too so in all cases you'd need some MIDI editing to get the sound you'd expect).
If you accept however to do some MIDI editing and/or some SoundFont editing manutally, then the answer to both of these questions is of course yes, and GBAMusRiper will probably be a great help.

Q : Can I also rip voices/sound effects ?
A : Yes absolutely. In fact the engine does not differenciate music, sound effects and voices in any way so you'll rip them all automatically.

Q : The first song/first few songs works fine, but the following songs doesn't work.
A : This is probably a problem with using various sound font banks. You should be sure the driver and MIDI player you use supports banks. It it doesn't work in normal (GS) mode try to use the -xg argument for XG mode maybe you'll get better results.

Q : The songs works usually fine but they sometimes sound wrong
A : Maybe your driver doesn't support the GS message to disable "drums" on midi channel 10. The way I rip the sound font requires that no channel is ever using MDI "drums" (even if there is drums in the song - they'll be mapped to a normal instrument). You should try the -rc (rearrange channels) and/or -xg options.

Q : Some Game Boy square wave part is missing in some songs in some games.
A : Again this is (sadly) normal. It means there is a supposedly "unused" instrument which in fact is used. The easiest fix is to fix the MIDIs in order to locate the part which don't play and change it's instrument to another equivalent one. Another approach would be to create the preset manually in the Sound Font file by copying another existing equivalent instrument.

Q : Why does it takes so long to rip the songs/soundfont ?
A : It really depends on the game. Some games use a different sound bank for every single song, so those games takes a very long time to rip. In all cases it's normal the conversion can't be done instantly.

Q : Can you adapt this riper to work with Game X which doesn't use the sappy sound engine ?
Q : Can you make this riper work with GameBoy Color / Nintendo DS / anyother console games ?
A : No, because other sound engines works completely differently.

Q : The soundfonts causes major trouble / crashes the program when I'm trying to use it
A : Some games uses too many different soundbanks for different songs and this causes problems in extreme cases (more than 128 banks). You should use the -sb command to separate sound banks into different folders, which will result in many sound font files instead of a single sound font with many sound banks.

=== Plans for future updates ===
- Any suggestions are welcome
- Could someone make a nice GUI for this program ? It would be amazing ! And personally I have zero knowledge in GUI programming.

=== Contact ===
Contact me at jmasur at bluewin dot ch if you want to give ideas about how I can improve GBA Mus Riper, or even better, if you improved the program by yourself.

=== History ===
v1.0 july 2012
First public release

v1.1 august 2012
Added -raw and -sb options

v2.0 january 2014
Major update. The program was completely rewritten in C++ so that it's cleaner, much faster and there is no need for the Java virtual machine anymore.
Windows users will find pre-compiled .exe files, while linux users will have to compile it manually (see the compile notes).
Pokemon ROMs are now supported (despite the fact I absolutely loathe pokemons) because I was asked to support them dozen of times.
Other games that weren't supported like Metroid games can now be ripped manually (see FAQ).

v2.1 january 2014
A small but significant fix. Prevents instruments from being duplicated if the same instrument is present 2 (or more) times in the ROM, this copy won't be reflected in the Sound Font.
The bank that overlap are detected and instrument that are already in an other bank are suppressed, so this reduces drastically the # of instruments in games with lots of small banks instead of several large banks.

v2.2 december 2014
Added support for Golden Sun synth instruments, thanks to ipatix for this one. Note that they do not sound exactly like their original counterpart, unfortunately, just like PSG instrument it's impossible to replicate them 1:1 with the SF2 format.
Added a new sappy engine seracher by loveemu, more efficient than my own. Fixed a bug in MIDI generation that would cause RPN and NRPN conflicts.
Fixed a bug with vibrato at the start of tracks (well I hacked a solution to it instead of properly fixing the problem actually).

v2.3 march 2015
Just a fixes of a few bugs with missing instruments. Pokémon, whose support was broken in 2.2 is again supported.

v2.4 april 2016
A few bugfixes, BDPCM comrpessed samples are supported