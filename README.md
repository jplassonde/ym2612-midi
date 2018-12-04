# ym2612-midi
Keyboard controlled YM2612

File: main.c
	Initialize peripherals, launch child progress and get commands via a pipe to
	control the sound chip(s) through an I/O expander.
	Build the frequency table at runtime based on the defined clock.
	Support up to 5 YM2612/YM3438 (30 channels), by connecting them on the bus,
	connecting the chip select to the next free I/O expander port B pin, and modifying 
	the CHIP_COUNT constant. 

File: chanqueue.c
	Circular queue implementation for channel pointers.
	This way, by re-using the "oldest" available channel first, keys are more likely
	to go through their whole note-release phase.

File: instruments.c
	Count the number of .dmp (Deflemask patches) file in the patches folder, allocate
	memory for them then parses them in structures. Using this file format
	allows to design instruments easily using Deflemask tracker.

File: midiin.c 
	Child process program, read from MIDI, filter off the Alive signal
	and send a 3 bytes command to the main program on events. Now keeps track
	of the number of channels used and send an "all off" command when all channels
	should be off to mitigate issues with UART. (a slight bitrate difference
	between the 2 devices causes notes to get stuck a few times per hour)

Structs & constants definition and function declaration in their respective header. 
main.h, channel.h, instruments.h
