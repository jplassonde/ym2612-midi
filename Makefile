CC=arm-dey-linux-gnueabi-gcc -march=armv7-a -marm -mthumb-interwork -mfloat-abi=hard \
   -mfpu=neon -mtune=cortex-a7 --sysroot=/opt/dey/2.0-r5/sysroots/cortexa7hf-vfp-neon-dey-linux-gnueabi

default:
	make ymsynth
	make midiin

ymsynth:
	${CC} main.c instruments.c chanqueue.c -o ymsynth

midiin:
	 ${CC} midiin.c -o midiin

