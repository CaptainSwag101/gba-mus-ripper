@echo OFF
mkdir debug
mkdir release
cd sappy_detector\
robocopy .\debug ..\debug sappy_detector.exe
robocopy .\release ..\release sappy_detector.exe
cd ..
cd song_ripper\
robocopy .\debug ..\debug song_ripper.exe
robocopy .\release ..\release song_ripper.exe
cd ..
cd sound_font_ripper\
robocopy .\ ..\debug goldensun_synth.raw
robocopy .\ ..\release goldensun_synth.raw
robocopy .\ ..\debug psg_data.raw
robocopy .\ ..\release psg_data.raw
robocopy .\debug ..\debug sound_font_ripper.exe
robocopy .\release ..\release sound_font_ripper.exe
cd ..
cd gba_mus_ripper\
robocopy .\debug ..\debug gba_mus_ripper.exe
robocopy .\release ..\release gba_mus_ripper.exe
cd ..