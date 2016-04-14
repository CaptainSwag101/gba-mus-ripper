@echo OFF
mkdir debug
mkdir release
cd sappy_detector/
cp ./debug/sappy_detector ../debug
cp ./release/sappy_detector ../release
cd ..
cd song_ripper/
cp ./debug/song_ripper ../debug
cp ./release/song_ripper ../release
cd ..
cd sound_font_ripper/
cp ./debug/goldensun_synth.raw ../debug
cp ./release/goldensun_synth.raw ../release
cp ./debug/psg_data.raw ../debug
cp ./release/psg_data.raw ../release
cp ./debug/sound_font_ripper ../debug
cp ./release/sound_font_ripper ../release
cd ..
cd gba_mus_ripper/
cp ./debug/gba_mus_ripper ../debug
cp ./release/gba_mus_ripper ../release
cd ..
cd gba_mus_ripper_gui/
cp ./debug/gba_mus_ripper_gui ../debug
cp ./release/gba_mus_ripper_gui ../release
cd ..
# ./debug/ ../debug
# ./release/ ../release
