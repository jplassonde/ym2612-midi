#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <asm-generic/termbits.h>

#define KEY 0x90
#define CTRL_CHNG 0xB0
#define ALIVE_SIG 0xFE
#define DEBUG
#define MAX_VELOCITY 0x7F

uint8_t get_byte(int fd);

int main(int argc, char ** argv)  {
	int pipe_fd = atoi(argv[1]);
	char send_buffer[3];
	int fd;
	uint8_t command;
	uint8_t key;
	uint8_t velocity;
	uint8_t instrument;
	uint8_t voice_count = 0

	if ((fd = open("/dev/ttymxc3", O_RDWR | O_NOCTTY | O_NDELAY)) < 0) {
		exit(1);
	}

	struct termios2 tio;

	ioctl(fd, TCGETS2, &tio);
	tio.c_cflag &= ~CBAUD;
	tio.c_cflag |= BOTHER;
	tio.c_ispeed = 31250;
	tio.c_ospeed = 31250;
	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	ioctl(fd, TCSETS2, &tio);
	fcntl(fd, F_SETFL, 0);
	
	while(1) {
		command = get_byte(fd);

		if (command == ALIVE_SIG) 
			continue;
		else if (command == 0x90) {
			key = get_byte(fd);
			velocity = get_byte(fd);	
			
			if (velocity != 0) {
				send_buffer[0] = 0;
				send_buffer[1] = key;
				send_buffer[2] = (MAX_VELOCITY - velocity)/5;
				write(pipe_fd, send_buffer, 3);
				++voice_count;

#ifdef DEBUG
				printf("Key on - Key: %x Velocity: %x\n", key, velocity);
#endif
			} else {	
				send_buffer[0] = 1;
				send_buffer[1] = key;
				write(pipe_fd, send_buffer, 3);
				--voice_count;

				if (voice_count == 0) {
					send_buffer[0] = 3;
					write(pipe_fd, send_buffer, 3);
				}
#ifdef DEBUG
				printf("Key off - Key: %x\n", key);
#endif
			}
		} else if (command == 0xB0) {
			for (int i = 0; i < 6; i++) {
				instrument = get_byte(fd);	
			}
			send_buffer[0] = 2;
			send_buffer[1] = instrument;
			write(pipe_fd, send_buffer, 3);
#ifdef DEBUG
			printf("Control change - Value: %x\n",  instrument);
#endif
		} else {
			key = command;
			velocity = get_byte(fd);
			if (velocity != 0) {
				send_buffer[0] = 0;
				send_buffer[1] = key;
				send_buffer[2] = (MAX_VELOCITY - velocity)/5;
				write(pipe_fd, send_buffer, 3);
				
				++voice_count;
#ifdef DEBUG
				printf("Key on - Key: %x Velocity: %x\n", key, velocity);
#endif
			} else {
				send_buffer[0] = 1;
				send_buffer[1] = key;
				write(pipe_fd, send_buffer, 3);
				
				--voice_count;
                if (voice_count == 0) {
                    send_buffer[0] = 3;
                    write(pipe_fd, send_buffer, 3);
                }

#ifdef DEBUG
				printf("Key off - Key: %x\n", key);
#endif
			}
		}
	}
}

uint8_t get_byte(int fd) {
	uint8_t byte;
	while (read(fd, &byte, 1) < 1);
	return byte; 
}	
