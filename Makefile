# On Windows
#CPPC=i686-w64-mingw32-g++.exe -std=gnu++11
#CC=i686-w64-mingw32-gcc.exe -std=c99

# On Linux
CPPC=g++ -std=gnu++11
CC=gcc -std=c99

# Parameters used for compilation
FLAGS=-Wall -fdata-sections -ffunction-sections -fmax-errors=5 -Os
# Additional parameters used for linking whole programs
WHOLE=-s -fwhole-program -static

all: $(shell mkdir -p out) out/sappy_detector out/song_riper out/sound_font_riper out/gba_mus_riper

out/sappy_detector: sappy_detector.c
	$(CC) $(FLAGS) $(WHOLE) sappy_detector.c -o out/sappy_detector

out/song_riper: song_riper.cpp midi.hpp midi.o
	$(CPPC) $(FLAGS) $(WHOLE) song_riper.cpp midi.o -o out/song_riper

out/sound_font_riper: sound_font_riper.o gba_samples.o gba_instr.o sf2.o
	$(CPPC) $(FLAGS) $(WHOLE) gba_samples.o gba_instr.o sf2.o sound_font_riper.o -o out/sound_font_riper

out/gba_mus_riper: gba_mus_riper.cpp hex_string.hpp
	$(CPPC) $(FLAGS) $(WHOLE) gba_mus_riper.cpp -o out/gba_mus_riper

midi.o: midi.cpp midi.hpp
	$(CPPC) $(FLAGS) -c midi.cpp -o midi.o

gba_samples.o : gba_samples.cpp gba_samples.hpp hex_string.hpp sf2.hpp sf2_types.hpp
	$(CPPC) $(FLAGS) -c gba_samples.cpp -o gba_samples.o

gba_instr.o : gba_instr.cpp gba_instr.hpp sf2.hpp sf2_types.hpp hex_string.hpp gba_samples.hpp
	$(CPPC) $(FLAGS) -c gba_instr.cpp -o gba_instr.o

sf2.o : sf2.cpp sf2.hpp sf2_types.hpp sf2_chunks.hpp
	$(CPPC) $(FLAGS) -c sf2.cpp -o sf2.o

sound_font_riper.o: sound_font_riper.cpp sf2.hpp gba_instr.hpp hex_string.hpp
	$(CPPC) $(FLAGS) -c sound_font_riper.cpp -o sound_font_riper.o

clean:
	rm -f *.o *.s *.i *.ii
	rm -rf out/
