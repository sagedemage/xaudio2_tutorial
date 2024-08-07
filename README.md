# xaudio2_tutorial
Demo of a windows desktop window playing a WAVE file via XAudio2

![Windows Desktop Window](./screenshots/windows_desktop_window.webp)

## WAVE Soundfile Format

The WAVE file format is a part of the **Microsoft's RIFF specification** for storing multimedia files. A WAVE file is typically a **RIFF** file with a "WAVE" chunk which consists of two sub-chunks: 
1. **fmt**: Specifies the data format
2. **data**:  contains the actual sample data

![The Canonical WAVE file format](./images/wav-sound-format.webp)

Image Resource: [The Canonical WAVE file format](http://soundfile.sapp.org/doc/WaveFormat/wav-sound-format.gif)

More information about the WAVE PCM soundfile format is located [here](http://soundfile.sapp.org/doc/WaveFormat/).